/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#pragma once

.macro func name
    .global \name
\name:
.endm

.macro endfunc name
.endm

.macro isb
    isb
.endm

.macro dsb sy
    dsb \sy
.endm

.macro dmb sy
    dmb \sy
.endm
