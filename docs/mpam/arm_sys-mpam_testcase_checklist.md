# SYS-MPAM Rule Coverage Checklist

This document describes the SYS-MPAM rule coverage checklist and status categories.
The coverage matrix itself is maintained in JSON form and generated as CSV so it
can be downloaded, filtered, and reviewed without duplicating the full table here.

## Current Summary

| Status | Count |
| --- | ---: |
| `COVERED` | 137 |
| `NOT_COVERED` | 76 |
| `TOTAL` | 213 |

## Platform Information

Platform applicability is tracked per rule in the generated CSV.

## Coverage Matrix

The full checklist is available here:

[Download SYS-MPAM testcase checklist CSV](arm_sys-mpam_testcase_checklist.csv)

Gitiles may display the CSV as plain text. Download the file and open it in
Excel, LibreOffice, or another spreadsheet tool for filtering and offline review.

The CSV contains these columns:

| Column | Description |
| --- | --- |
| `Specification Rule ID` | Architecture rule ID. |
| `BM` | Whether the rule is applicable to this platform. |
| `UEFI` | Whether the rule is applicable to this platform. |
| `Status` | Current ACS coverage classification. |
| `Tests` | Direct ACS test file or indirect coverage source. |
| `Notes` | Short reasoning for the classification. |
| `Rule Text` | Full architecture rule text. |

## Status Definitions

| Status | Meaning |
| --- | --- |
| `COVERED` | Rule is closed for ACS coverage: directly covered, indirectly covered, or no standalone ACS check is required. See Notes for the reason. |
| `NOT_COVERED` | Rule is not closed by ACS coverage. It is either not testable by ACS or is a coverage gap. See Notes for the reason. |

## Maintenance

Update `arm_sys-mpam_testcase_checklist.json` when rule coverage changes, then regenerate this
Markdown file and the CSV with:

```bash
python3 tools/scripts/generate_rule_checklist.py --json docs/mpam/arm_sys-mpam_testcase_checklist.json
```

To choose explicit output paths:

```bash
python3 tools/scripts/generate_rule_checklist.py --json docs/mpam/arm_sys-mpam_testcase_checklist.json --md docs/mpam/arm_sys-mpam_testcase_checklist.md --csv docs/mpam/arm_sys-mpam_testcase_checklist.csv
```

Direct executable mappings must still be represented in `rule_metadata.c`.
Coverage detail is captured in `Notes` using prefixes such as `Direct:`,
`Indirect:`, `No ACS check required:`, `Not testable:`, and `Gap:`.
