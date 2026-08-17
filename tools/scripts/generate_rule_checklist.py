## @file
 # Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 # SPDX-License-Identifier : Apache-2.0
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##

"""Generate rule coverage checklist Markdown and CSV from JSON."""

from __future__ import annotations

import argparse
import csv
import json
from collections import Counter
from pathlib import Path


DEFAULT_STATUS_ORDER = [
    "COVERED",
    "NOT_COVERED",
]

BASE_CSV_FIELDNAMES = [
    "Specification Rule ID",
    "Status",
    "Tests",
    "Notes",
    "Rule Text",
]


REPO_ROOT = Path(__file__).resolve().parents[2]


def resolve_path(path: Path) -> Path:
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def display_path(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


def load_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as json_file:
        return json.load(json_file)


def status_order(data: dict) -> list[str]:
    return data.get("metadata", {}).get("status_order", DEFAULT_STATUS_ORDER)


def validate(data: dict) -> None:
    statuses = set(data["status_definitions"])
    rule_ids = []

    for rule in data["rules"]:
        rule_ids.append(rule["rule_id"])
        if rule["status"] not in statuses:
            raise ValueError(f"Unknown status for {rule['rule_id']}: {rule['status']}")

        for sub_rule in rule.get("sub_rules", []):
            if sub_rule["status"] not in statuses:
                raise ValueError(
                    f"Unknown status for {rule['rule_id']}/{sub_rule['sub_rule_id']}: "
                    f"{sub_rule['status']}"
                )

    if len(rule_ids) != len(set(rule_ids)):
        raise ValueError("Duplicate rule_id entries found")


def has_sub_rules(data: dict) -> bool:
    return any(rule.get("sub_rules") for rule in data["rules"])


def platform_names(data: dict) -> list[str]:
    return data.get("metadata", {}).get("platforms", [])


def csv_fieldnames(data: dict) -> list[str]:
    fieldnames = BASE_CSV_FIELDNAMES.copy()
    insert_at = 1
    if has_sub_rules(data):
        fieldnames.insert(insert_at, "Sub Rule ID")
        insert_at += 1

    for platform in platform_names(data):
        fieldnames.insert(insert_at, platform)
        insert_at += 1

    return fieldnames


def platform_columns(data: dict, entry: dict) -> dict:
    platforms = entry.get("platforms", [])
    if isinstance(platforms, dict):
        applicable = {
            platform
            for platform, value in platforms.items()
            if str(value).lower() in {"yes", "true", "1"}
        }
    else:
        applicable = set(platforms)

    return {
        platform: "Yes" if platform in applicable else "No"
        for platform in platform_names(data)
    }


def rule_rows(data: dict) -> list[dict]:
    rows = []
    for rule in data["rules"]:
        sub_rules = rule.get("sub_rules", [])
        if sub_rules:
            for sub_rule in sub_rules:
                rows.append(
                    {
                        "Specification Rule ID": rule["rule_id"],
                        "Sub Rule ID": sub_rule["sub_rule_id"],
                        "Status": sub_rule["status"],
                        **platform_columns(data, sub_rule),
                        "Tests": ", ".join(sub_rule.get("tests", [])) or "-",
                        "Notes": sub_rule.get("notes", ""),
                        "Rule Text": sub_rule.get("rule_text", rule.get("rule_text", "")),
                    }
                )
        else:
            rows.append(
                {
                    "Specification Rule ID": rule["rule_id"],
                    "Sub Rule ID": "-",
                    "Status": rule["status"],
                    **platform_columns(data, rule),
                    "Tests": ", ".join(rule.get("tests", [])) or "-",
                    "Notes": rule.get("notes", ""),
                    "Rule Text": rule.get("rule_text", ""),
                }
            )
    return rows


def write_csv(data: dict, path: Path) -> None:
    rows = rule_rows(data)
    with path.open("w", encoding="utf-8", newline="") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=csv_fieldnames(data), extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def write_markdown(data: dict, json_path: Path, csv_path: Path, path: Path) -> None:
    metadata = data["metadata"]
    rules = data["rules"]
    counts = Counter(rule["status"] for rule in rules)
    order = status_order(data)
    csv_name = csv_path.name
    json_display = display_path(json_path)
    md_display = display_path(path)
    csv_display = display_path(csv_path)
    generator_display = display_path(Path(__file__).resolve())
    architecture = metadata.get("architecture", "the architecture")
    download_label = metadata.get("csv_download_label", "Download rule coverage checklist CSV")
    include_sub_rule_id = has_sub_rules(data)
    platforms = platform_names(data)

    lines = [
        f"# {metadata['title']}",
        "",
        f"This document describes the {architecture} rule coverage checklist and status categories.",
        "The coverage matrix itself is maintained in JSON form and generated as CSV so it",
        "can be downloaded, filtered, and reviewed without duplicating the full table here.",
        "",
        "## Current Summary",
        "",
        "| Status | Count |",
        "| --- | ---: |",
    ]
    for status in order:
        lines.append(f"| `{status}` | {counts.get(status, 0)} |")
    lines.extend(
        [
            f"| `TOTAL` | {len(rules)} |",
            "",
        ]
    )

    if platforms:
        lines.extend(
            [
                "## Platform Information",
                "",
                "Platform applicability is tracked per rule in the generated CSV.",
                "",
            ]
        )

    lines.extend(
        [
            "## Coverage Matrix",
            "",
            "The full checklist is available here:",
            "",
            f"[{download_label}]({csv_name})",
            "",
            "Gitiles may display the CSV as plain text. Download the file and open it in",
            "Excel, LibreOffice, or another spreadsheet tool for filtering and offline review.",
            "",
            "The CSV contains these columns:",
            "",
            "| Column | Description |",
            "| --- | --- |",
            "| `Specification Rule ID` | Architecture rule ID. |",
            *(["| `Sub Rule ID` | Optional sub-rule ID when a rule is split into smaller checks. |"] if include_sub_rule_id else []),
            *([f"| `{platform}` | Whether the rule is applicable to this platform. |" for platform in platforms]),
            "| `Status` | Current ACS coverage classification. |",
            "| `Tests` | Direct ACS test file or indirect coverage source. |",
            "| `Notes` | Short reasoning for the classification. |",
            "| `Rule Text` | Full architecture rule text. |",
            "",
            "## Status Definitions",
            "",
            "| Status | Meaning |",
            "| --- | --- |",
        ]
    )

    for status in order:
        details = data["status_definitions"][status]
        lines.append(f"| `{status}` | {details['meaning']} |")

    lines.extend(
        [
            "",
            "## Maintenance",
            "",
            f"Update `{json_path.name}` when rule coverage changes, then regenerate this",
            "Markdown file and the CSV with:",
            "",
            "```bash",
            f"python3 {generator_display} --json {json_display}",
            "```",
            "",
            "To choose explicit output paths:",
            "",
            "```bash",
            f"python3 {generator_display} --json {json_display} --md {md_display} --csv {csv_display}",
            "```",
            "",
            "Direct executable mappings must still be represented in `rule_metadata.c`.",
            "Coverage detail is captured in `Notes` using prefixes such as `Direct:`,",
            "`Indirect:`, `No ACS check required:`, `Not testable:`, and `Gap:`.",
            "",
        ]
    )

    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--json",
        type=Path,
        default=Path("docs/mpam/arm_sys-mpam_testcase_checklist.json"),
        help="Input JSON checklist. Defaults to the SYS-MPAM checklist.",
    )
    parser.add_argument(
        "--md",
        type=Path,
        help="Output Markdown path. Defaults to the input JSON path with .md suffix.",
    )
    parser.add_argument(
        "--csv",
        type=Path,
        help="Output CSV path. Defaults to the input JSON path with .csv suffix.",
    )
    args = parser.parse_args()

    json_path = resolve_path(args.json)
    md_path = resolve_path(args.md) if args.md else json_path.with_suffix(".md")
    csv_path = resolve_path(args.csv) if args.csv else json_path.with_suffix(".csv")

    data = load_json(json_path)
    validate(data)
    write_csv(data, csv_path)
    write_markdown(data, json_path, csv_path, md_path)


if __name__ == "__main__":
    main()
