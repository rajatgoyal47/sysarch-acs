#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
set -euo pipefail

ACS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

OUT="${1:-${ACS_DIR}/build/fip.bin}"

FIPTOOL="${FIPTOOL:-fiptool}"
BL31_BIN="${BL31_BIN:-}"
BL32_BIN="${BL32_BIN:-${ACS_DIR}/build/secure.bin}"   # tos-fw
BL33_BIN="${BL33_BIN:-${ACS_DIR}/build/nsel2.bin}"    # nt-fw
REL2_BIN="${REL2_BIN:-${ACS_DIR}/build/rel2.bin}"     # rmm-fw (optional)
TOS_FW_CONFIG="${TOS_FW_CONFIG:-}"                       # tos-fw-config (optional)

if [[ -z "${BL31_BIN}" ]]; then
    echo "BL31_BIN is required (path to TF-A bl31.bin)" >&2
    exit 1
fi

if [[ ! -f "${BL31_BIN}" ]]; then
    echo "BL31_BIN not found: ${BL31_BIN}" >&2
    exit 1
fi

if [[ ! -f "${BL32_BIN}" ]]; then
    echo "BL32_BIN not found: ${BL32_BIN}" >&2
    exit 1
fi

if [[ ! -f "${BL33_BIN}" ]]; then
    echo "BL33_BIN not found: ${BL33_BIN}" >&2
    exit 1
fi

mkdir -p "$(dirname "${OUT}")"

echo "Creating FIP:"
echo "  BL31: ${BL31_BIN}"
echo "  BL32: ${BL32_BIN}"
echo "  BL33: ${BL33_BIN}"
if [[ -f "${REL2_BIN}" ]]; then
    echo "  REL2: ${REL2_BIN}"
fi
if [[ -n "${TOS_FW_CONFIG}" ]]; then
    echo "  TOS_FW_CONFIG: ${TOS_FW_CONFIG}"
fi
echo "  OUT : ${OUT}"

args=(
    create
    --soc-fw "${BL31_BIN}"
    --tos-fw "${BL32_BIN}"
    --nt-fw "${BL33_BIN}"
)

if [[ -f "${REL2_BIN}" ]]; then
    args+=(--rmm-fw "${REL2_BIN}")
fi

if [[ -n "${TOS_FW_CONFIG}" ]]; then
    if [[ ! -f "${TOS_FW_CONFIG}" ]]; then
        echo "TOS_FW_CONFIG not found: ${TOS_FW_CONFIG}" >&2
        exit 1
    fi
    args+=(--tos-fw-config "${TOS_FW_CONFIG}")
fi

"${FIPTOOL}" "${args[@]}" "${OUT}"
