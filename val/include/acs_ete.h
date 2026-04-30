/** @file
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __ACS_ETE_H
#define __ACS_ETE_H

#define ACS_TRBLIMITR_EL1_E     (1 << 0)
#define ACS_TRBLIMITR_EL1_nVM   (1 << 5)
#define ACS_TRBLIMITR_EL1_XE    (1 << 6)

/* Trace Packet Header */

#define TRACE_ALIGNMENT_PKT               0x00  /*  Alignment Packet  */
#define TRACE_INFO_PKT                    0x01  /*  Trace Info Packet */
#define TRACE_TIMESTAMP_V1_PKT            0x02  /*  Timestamp Packet (Format 1)  */
#define TRACE_TIMESTAMP_V2_PKT            0x03  /*  Timestamp Packet (Format 2)  */
#define TRACE_TRACE_ON_PKT                0x04  /*  Trace On Packet  */
#define TRACE_EXCEPTION_PKT               0x06  /*  Exception Packet */
#define TRACE_INSTRUMENT_PKT              0x09  /*  Instrumentation Packet    */
#define TRACE_TRANSACTION_START_PKT       0x0A  /*  Transaction Start Packet  */
#define TRACE_TRANSACTION_COMMIT_PKT      0x0B  /*  Transaction Commit Packet */
#define TRACE_CC_F2_0_SMALL_COMMIT_PKT    0x0C  /*  Cycle Count Format 2, Small Commit Packet  */
#define TRACE_CC_F2_1_PKT                 0x0D  /*  Cycle Count Format 2.1 Packet        */
#define TRACE_CC_F1_X_COUNT_PKT           0x0E  /*  Cycle Count Format 1, known count    */
#define TRACE_CC_F1_X_UNK_COUNT_PKT       0x0F  /*  Cycle Count Format 1, unknown count  */
#define TRACE_COMMIT_PKT                  0x2D  /*  Commit Packet  */

#define TRACE_EXACT_MATCH_ADDR_PKT        0x24  /*  Exact Match Address Packet                 */
#define EXCEPTION_SHORT_ADDR_PKT          0x25  /*  Exception Packet with Short Add (IS0/IS1)  */
#define EXCEPTION_32BIT_ADDR_PKT          0x26  /*  Exception Packet with 32-bit Add (IS0/IS1) */
#define EXCEPTION_64BIT_ADDR_PKT          0x27  /*  Exception Packet with 64-bit Add (IS0/IS1) */

#define TRACE_IGNORE_PKT                  0x70  /*  Ignore Packet (Event or Placeholder)       */
#define TRACE_CONTEXT_SAME_PKT            0x80  /*  Same Context Packet        */
#define TRACE_CONTEXT_PKT                 0x81  /*  Context Packet (2 bytes)   */
#define TRACE_TIMESTAMP_MARKER_PKT        0x88  /*  Timestamp Marker Packet    */
#define TRACE_Q_PKT                       0xAF  /*  Q Packet (Compressed or Composite trace)   */

#define CTX_32BIT_IS0_PKT                 0x82  /*  Target Address with Context 32-bit IS0/IS1 */
#define CTX_32BIT_IS1_PKT                 0x83  /*  Target Address with Context 32-bit IS0/IS1 */
#define CTX_64BIT_IS0_PKT                 0x85  /*  Target Address with Context 64-bit IS0/IS1 */
#define CTX_64BIT_IS1_PKT                 0x86  /*  Target Address with Context 64-bit IS0/IS1 */

#define TARGET_ADDR_SHORT_IS0_PKT         0x95  /*  Target Address Short IS0 Packet  */
#define TARGET_ADDR_SHORT_IS1_PKT         0x96  /*  Target Address Short IS1 Packet  */

#define TARGET_ADDR_32BIT_IS0_PKT         0x9A  /*  Target Address 32-bit IS0 Packet */
#define TARGET_ADDR_32BIT_IS1_PKT         0x9B  /*  Target Address 32-bit IS1 Packet */

#define TARGET_ADDR_64BIT_IS0_PKT         0x9D  /*  Target Address 64-bit IS0 Packet */
#define TARGET_ADDR_64BIT_IS1_PKT         0x9E  /*  Target Address 64-bit IS1 Packet */

#define Q_SHORT_ADDR_IS0_PKT              0xA5  /*  Q short address IS0 Packet  */
#define Q_SHORT_ADDR_IS1_PKT              0xA6  /*  Q short address IS1 Packet  */

#define Q_32BIT_ADDR_IS0_PKT              0xAA  /*  Q 32-bit address IS0 Packet  */
#define Q_32BIT_ADDR_IS1_PKT              0xAB  /*  Q 32-bit address IS1 Packet  */

#define Q_COUNT_PKT                       0xAC  /*  Q with count Packet (Commit)  */

#define SRC_SHORT_ADDR_IS0_PKT            0xB4  /*  Source Address Short IS0 Packet  */
#define SRC_SHORT_ADDR_IS1_PKT            0xB5  /*  Source Address Short IS1 Packet  */

#define SRC_32BIT_ADDR_IS0_PKT            0xB6  /*  Source Address 32-bit IS0 Packet  */
#define SRC_32BIT_ADDR_IS1_PKT            0xB7  /*  Source Address 32-bit IS1 Packet  */

#define SRC_64BIT_ADDR_IS0_PKT            0xB8  /*  Source Address 64-bit IS0 Packet  */
#define SRC_64BIT_ADDR_IS1_PKT            0xB9  /*  Source Address 64-bit IS1 Packet  */


/* Trace Header Value and Mask */

#define CYCLE_COUNT_FORMAT_3_1_MASK       0xFC
#define CYCLE_COUNT_FORMAT_3_1_VAL        0x10  /*  Cycle Count Format 3_1 Packet */

#define MISPREDICT_PKT_MASK               0xFC
#define MISPREDICT_PKT_VAL                0x30  /*  Mispredict Packet */

#define CANCEL_FORMAT_2_PKT_MASK          0xFC
#define CANCEL_FORMAT_2_PKT_VAL           0x34  /*  Cancel Format 2 Packet */

#define CANCEL_FORMAT_3_PKT_MASK          0xF8
#define CANCEL_FORMAT_3_PKT_VAL           0x38  /*  Cancel Format 3 Packet */

#define EVENT_PKT_MASK                    0xF0
#define EVENT_PKT_VAL                     0x70  /*  Event Packet */

#define TARGET_ADDR_EXACT_MATCH_MASK      0xFC
#define TARGET_ADDR_EXACT_MATCH_VAL       0x90  /*  Target Address Exact Match Packet */

#define SRC_ADDR_EXACT_MATCH_MASK         0xFC
#define SRC_ADDR_EXACT_MATCH_VAL          0xB0  /*  Source Address Exact Match Packet */

#define ATOM_FORMAT_X_PKT_MASK            0xC0
#define ATOM_FORMAT_X_PKT_VAL             0xC0  /*  Atom Format X Packet */

#define CYCLE_COUNT_FORMAT_3_0_MASK       0xF0
#define CYCLE_COUNT_FORMAT_3_0_VAL        0x10  /*  Cycle Count Format 3_0 Packet */

#define CANCEL_FORMAT_1_PKT_MASK          0xFE
#define CANCEL_FORMAT_1_PKT_VAL           0x2E  /*  Cancel Format 1 Packet */

#define Q_EXACT_MATCH_PKT_MASK            0xFC
#define Q_EXACT_MATCH_PKT_VAL             0xA0  /*  Q with Exact Match Address Packet */

/* Trace Packet Length */

#define DISCARD_OVERFLOW_PKT_LEN        0x02
#define ALIGN_SYNC_PKT_LEN              0x0C
#define TRACE_INFO_PKT_LEN              0x02
#define TRACE_PKT_MIN_LEN               0x01
#define TRACE_EXCEPTION_32_PKT_LEN      0x08
#define TRACE_EXCEPTION_64_PKT_LEN      0x0C
#define TRACE_SHORT_PKT_LEN             0x03
#define EXCEPTION_SHORT_ADDR_PKT_LEN    0x05
#define EXCEPTION_32_ADDR_PKT_LEN       0x07
#define EXCEPTION_64_ADDR_PKT_LEN       0x0B
#define TRACE_INSTRUMENT_PKT_LEN        0x0A
#define TRACE_CC_F2_PKT_LEN             0x02
#define TARGET_ADDR_32BIT_ISX_PKT_LEN   0x05
#define TARGET_ADDR_64BIT_ISX_PKT_LEN   0x09
#define Q_32BIT_ADDR_IS0_PKT_A_LEN      0x05
#define SRC_32BIT_ADDR_PKT_LEN          0x05
#define SRC_64BIT_ADDR_PKT_LEN          0x09

/* Tace Packet Layout Length */

#define CC_LAYOUT_LEN                   0x01
#define CYCT_LAYOUT_LEN                 0x02
#define SPEC_LAYOUT_LEN                 0x05
#define VMID_LAYOUT_LEN                 0x04
#define CONTEXTID_LAYOUT_LEN            0x04



#define SH_TRACE_ENABLE_TRUE    1
#define SH_TRACE_ENABLE_FALSE   0

/* Timestamp */
#define TS_VALUE_MASK          0x7F
#define CONTINUITY_BIT_MASK    0x80

/* Timestamp Invalid */
#define TRACE_PKT_INVALID 0xFFFF

/* Trace Related Calls */
uint64_t trace_cbit_len(uint8_t *trace_stream, uint64_t index, uint64_t position,
                                     uint64_t layout_len);
uint64_t val_ete_get_trace_timestamp(uint64_t buffer_address);
uint64_t val_ete_generate_trace(uint64_t buffer_address, uint32_t self_hosted_trace_enabled);

uint64_t AA64GenerateETETrace(void);
uint64_t AA64EnableETETrace(void);
uint64_t AA64DisableETETrace(void);
uint64_t AA64SetupTraceAccess(void);
uint64_t AA64EnableTRBUTrace(uint32_t index, uint64_t buffer_addr, uint32_t trbu_mode);
uint64_t AA64DisableTRBUTrace(void);

void AA64EnableTFO(void);
void AA64DisableTFO(void);

uint32_t ete001_entry(uint32_t num_pe);
uint32_t ete002_entry(uint32_t num_pe);
uint32_t ete003_entry(uint32_t num_pe);
uint32_t ete004_entry(uint32_t num_pe);
uint32_t ete005_entry(uint32_t num_pe);
uint32_t ete006_entry(uint32_t num_pe);
uint32_t ete007_entry(uint32_t num_pe);
uint32_t ete008_entry(uint32_t num_pe);
uint32_t ete009_entry(uint32_t num_pe);

#endif // __ACS_ETE_H
