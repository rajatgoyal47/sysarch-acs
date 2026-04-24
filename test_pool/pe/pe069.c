/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/
#include "acs_val.h"
#include "acs_pe.h"
#include "val_interface.h"

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  69)
#define TEST_RULE  "S_L5PE_03"
#define TEST_DESC  "Check FEAT_TRF support per CS-BSA C  "

static void payload(void)
{
    uint32_t pe_family;
    uint64_t dfr0, pfr0, isar0, isar1, mmfr2;
    uint64_t ete, trbe;
    uint64_t dit, flagm, ids, lrcpc, at, trace_filt;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    pe_family = val_get_pe_architecture(index);
    if (pe_family == PROCESSOR_FAMILY_ARMV9) {
        val_print_primary_pe(DEBUG, "\n       Armv9 PE detected; skipping the test", 0, index);
        dfr0 = val_pe_reg_read(ID_AA64DFR0_EL1);

        /* ID_AA64DFR0_EL1.TraceVer[7:4] != 0 indicates FEAT_ETE support */
        ete = VAL_EXTRACT_BITS(dfr0, 4, 7);
        if (ete)
            val_print_primary_pe(DEBUG, "\n       FEAT_ETE SUPPORTED", 0, index);
        else
            val_print_primary_pe(DEBUG, "\n       FEAT_ETE NOT SUPPORTED", 0, index);
        val_print_primary_pe(DEBUG, " (TraceVer[7:4]=0x%llx)", ete, index);

        /* ID_AA64DFR0_EL1.TraceBuffer[47:44] != 0 indicates FEAT_TRBE support */
        trbe = VAL_EXTRACT_BITS(dfr0, 44, 47);
        if (trbe)
            val_print_primary_pe(DEBUG, "\n       FEAT_TRBE SUPPORTED", 0, index);
        else
            val_print_primary_pe(DEBUG, "\n       FEAT_TRBE NOT SUPPORTED", 0, index);
        val_print_primary_pe(DEBUG, " (TraceBuffer[47:44]=0x%llx)", trbe, index);
        val_set_status(index, RESULT_SKIP(1));
        return;
    } else if (pe_family == ACS_STATUS_ERR) {
        val_print_primary_pe(WARN, "\n       SMBIOS info missing, unable to determine if PE is v9",
                    0, index);
    }

    /* Approximate Armv8.4+ using mandatory features:
     * DIT>=1, TS>=1 (FlagM), LRCPC>=2, IDS>=1, AT>=1
     */
    pfr0 = val_pe_reg_read(ID_AA64PFR0_EL1);
    isar0 = val_pe_reg_read(ID_AA64ISAR0_EL1);
    isar1 = val_pe_reg_read(ID_AA64ISAR1_EL1);
    mmfr2 = val_pe_reg_read(ID_AA64MMFR2_EL1);

    dit   = VAL_EXTRACT_BITS(pfr0, 48, 51);
    flagm = VAL_EXTRACT_BITS(isar0, 52, 55);
    lrcpc = VAL_EXTRACT_BITS(isar1, 20, 23);
    ids   = VAL_EXTRACT_BITS(mmfr2, 36, 39);
    at    = VAL_EXTRACT_BITS(mmfr2, 32, 35);

    val_print_primary_pe(DEBUG, "\n       ID_AA64PFR0_EL1.DIT 0x%llx", dit, index);
    val_print_primary_pe(DEBUG, "\n       ID_AA64ISAR0_EL1.TS 0x%llx", flagm, index);
    val_print_primary_pe(DEBUG, "\n       ID_AA64MMFR2_EL1.IDS 0x%llx", ids, index);
    val_print_primary_pe(DEBUG, "\n       ID_AA64ISAR1_EL1.LRCPC 0x%llx", lrcpc, index);
    val_print_primary_pe(DEBUG, "\n       ID_AA64MMFR2_EL1.AT 0x%llx", at, index);

    if ((dit < 1) || (flagm < 1) || (ids < 1) || (lrcpc < 2) ||
        (at < 1)) {
        val_print_primary_pe(DEBUG, "\n       Skipping: PE does not meet Armv8.4 feature baseline",
                    0, index);
        val_set_status(index, RESULT_SKIP(2));
        return;
    }

    /* ID_AA64DFR0_EL1.TraceFilt[43:40] == 0b0001 indicates FEAT_TRF support */
    trace_filt = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64DFR0_EL1), 40, 43);

    if (trace_filt != 0x1) {
        val_print_primary_pe(ERROR, "\n       FEAT_TRF not implemented (TraceFilt[43:40]=0x%llx)",
                trace_filt, index);
        val_set_status(index, RESULT_FAIL(1));
        return;
    }

    val_print_primary_pe(INFO,
            "\n       FEAT_TRF supported; manual verification required for CS-BSA combination C",
            0, index);
    val_set_status(index, RESULT_PARTIAL_COVERED);
}

uint32_t pe069_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
