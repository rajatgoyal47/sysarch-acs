/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_firme_diag.h"
#include "val_firme_rules.h"
#include "val_firme_dispatch.h"
#include "val_firme_rmmd.h"
#include "val_common/val_smc_args.h"
#include "val_common/val_fault.h"
#include "val_firme_test.h"

/**
 *   @brief    - Complete an RMI dispatch request and return to RMMD.
 *   @param    - args   : In/out SMC argument frame.
 *   @param    - status : Completion status.
 *   @param    - out0   : Output word 0.
 *   @param    - out1   : Output word 1.
 *   @param    - out2   : Output word 2.
 *   @param    - out3   : Output word 3.
 *   @return   - void
**/
static void rmi_complete(smc_args_t *args,
                         uint64_t status,
                         uint64_t out0, uint64_t out1, uint64_t out2, uint64_t out3)
{
    /*
     * RMM returns to RMMD using RMM_RMI_REQ_COMPLETE.
     * RMMD forwards x1..x5 back to the Normal world as x0..x4.
     */
    args->x[0] = (uint64_t)RMM_RMI_REQ_COMPLETE;
    args->x[1] = status;
    args->x[2] = out0;
    args->x[3] = out1;
    args->x[4] = out2;
    args->x[5] = out3;
    args->x[6] = 0;
    args->x[7] = 0;
}

/**
 *   @brief    - Realm payload SMC entrypoint called by TF-A RMMD.
 *   @param    - args : In/out SMC argument frame (x0..x7).
 *   @return   - void
**/
void rel2_handle_smc(smc_args_t *args)
{
    uint64_t fid = 0;
    uint64_t cmd = 0;
    uint64_t a0 = 0;
    uint64_t a1 = 0;
    uint64_t a2 = 0;
    uint64_t a3 = 0;
    uint64_t status = (uint64_t)ACS_STATUS_PASS;
    uint64_t out0 = 0, out1 = 0, out2 = 0, out3 = 0;

    fid = args->x[0];

    if ((uint32_t)fid != (uint32_t)FIRME_ACS_RMI_DISPATCH) {
        rmi_complete(args, (uint64_t)ACS_STATUS_ERR, fid, 0, 0, 0);
        return;
    }

    cmd = args->x[1];
    a0 = args->x[2];
    a1 = args->x[3];
    a2 = args->x[4];
    a3 = args->x[5];

    if (cmd == (uint64_t)FIRME_ACS_CMD_PING) {
        out0 = 0x504f4e47ULL; /* 'PONG' */
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_DIAG) {
        firme_diag_out_t out = { 0 };

        status = (uint64_t)val_firme_diag_run(a0, a1, a2, a3, 0, &out);
        out0 = out.x0;
        out1 = out.x1;
        out2 = out.x2;
        out3 = out.x3;
    } else if (cmd == (uint64_t)FIRME_ACS_CMD_RUN_RULE) {
        firme_rule_res_t rr = {0};
        val_firme_rule_run_realm((firme_rule_id_t)a0, a1, &rr);
        status = rr.status;
        out0 = rr.fail_step;
        out1 = rr.last_x0;
        out2 = rr.last_x1;
    } else {
        status = (uint64_t)ACS_STATUS_ERR;
        out0 = cmd;
        out1 = a0;
        out2 = a1;
        out3 = a2 ^ a3;
    }

    rmi_complete(args, status, out0, out1, out2, out3);
}
