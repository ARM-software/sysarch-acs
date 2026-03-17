/** @file
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

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
#include "acs_common.h"
#include "acs_ete.h"
#include "val_interface.h"
#include "acs_pe.h"

/**
  @brief  Handles continuity bits in trace packets to determine the encoded length.

          1. Caller       - parse_tracestream API
          2. Prerequisite - A valid trace stream buffer

  @param  trace_stream - Pointer to the raw trace byte stream
  @param  index        - Base offset in the stream to start parsing from
  @param  position     - Position offset from index within the stream
  @param  layout_len   - Maximum length to scan (to avoid overruns)

  @return Number of bytes making up the field
**/

uint64_t trace_cbit_len(uint8_t *trace_stream, uint64_t index, uint64_t position,
          uint64_t layout_len)
{
  uint64_t i = 0;
  uint64_t len = 0;

   /* Loop through bytes while the continuation bit (C) is set.
      Stop when C=0 is found or maximum layout length is reached */
  while ((trace_stream[index + position + i] & CONTINUITY_BIT_MASK) && (i <  layout_len - 1)) {
      len++;
      i++;
  }

  return len+1; /* include the last byte without C=0 */
}

/**
  @brief  Parses trace stream to identify trace info packets and extract the timestamp header byte.

  @param  trace_bytes - Pointer to the raw trace stream buffer
  @param  trace_size  - Size of the trace stream in bytes

  @return Start byte index of the timestamp packet if found; otherwise, returns 0xFFFF
**/

uint64_t parse_tracestream(uint8_t *trace_bytes, uint64_t trace_size)
{
    uint64_t byte_index = 0;
    uint64_t pkt_len = 0;
    uint64_t pkt_type = 0;
    uint64_t length;
    uint64_t trcidr0_read = val_pe_reg_read(TRCIDR0);
    uint32_t pe_index = val_pe_get_index_mpid(val_pe_get_mpid());

    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Trace Size: %d ", trace_size, pe_index);

    /* Iterate through trace stream bytes until all are parsed */
    while (byte_index < trace_size) {
        val_print_primary_pe(ACS_PRINT_DEBUG, "\n       byte_value: %d ",
                                                         trace_bytes[byte_index], pe_index);
        val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Trace index of the byte value: %d ",
                                                         byte_index, pe_index);

        /* Identify and handle packet type based on current header packet byte */
        switch (trace_bytes[byte_index]) {

        case TRACE_ALIGNMENT_PKT:
            pkt_type = VAL_EXTRACT_BITS(trace_bytes[byte_index + 1], 0, 2);
            pkt_len = (pkt_type != 0) ? DISCARD_OVERFLOW_PKT_LEN : ALIGN_SYNC_PKT_LEN;
            break;

        case TRACE_INFO_PKT:
            pkt_len = TRACE_INFO_PKT_LEN;

            /* Check for CC field (bit 0) */
            if (VAL_EXTRACT_BITS(trace_bytes[byte_index + 1], 0, 0))
                pkt_len = pkt_len + CC_LAYOUT_LEN;

            /* Check for SPEC field (bit 2) */
            if (VAL_EXTRACT_BITS(trace_bytes[byte_index + 1], 2, 2))
                pkt_len = pkt_len + trace_cbit_len(trace_bytes, byte_index,
                                           pkt_len, SPEC_LAYOUT_LEN);

            /* Check for CYCT field (bit 3) */
            if (VAL_EXTRACT_BITS(trace_bytes[byte_index + 1], 3, 3))
                pkt_len = pkt_len + trace_cbit_len(trace_bytes, byte_index,
                                           pkt_len, CYCT_LAYOUT_LEN);
            break;

        case TRACE_TIMESTAMP_V1_PKT:
        case TRACE_TIMESTAMP_V2_PKT:
            return byte_index;

        case TRACE_TIMESTAMP_MARKER_PKT:
            pkt_len = TRACE_PKT_MIN_LEN;
            break;

        case TRACE_TRACE_ON_PKT:
        case TRACE_TRANSACTION_START_PKT:
        case TRACE_TRANSACTION_COMMIT_PKT:
        case TRACE_IGNORE_PKT:
        case TRACE_CONTEXT_SAME_PKT:
        case TRACE_Q_PKT:
            pkt_len = TRACE_PKT_MIN_LEN;
            break;

        case TRACE_EXCEPTION_PKT:
            uint8_t val2_full = VAL_EXTRACT_BITS(trace_bytes[byte_index + 2], 0, 7);
            uint8_t val2_partial = VAL_EXTRACT_BITS(trace_bytes[byte_index + 2], 2, 7);

            if (val2_full == 0x70) /* PE Reset or Transaction Failure Packet */
                pkt_len = TRACE_SHORT_PKT_LEN;
            else if (val2_full == 0x82 || val2_full == 0x83 ||
                     val2_full == 0x85 || val2_full == 0x86) {
                pkt_len = (val2_full < 0x85) ? TRACE_EXCEPTION_32_PKT_LEN :
                                               TRACE_EXCEPTION_64_PKT_LEN;

                if (VAL_EXTRACT_BITS(trace_bytes[byte_index + (pkt_len - 1)], 6, 6))
                    pkt_len = pkt_len + VMID_LAYOUT_LEN;

                if (VAL_EXTRACT_BITS(trace_bytes[byte_index + (pkt_len - 1)], 7, 7))
                    pkt_len = pkt_len + CONTEXTID_LAYOUT_LEN;
            }
            else {
                switch (val2_partial) {

                case TRACE_EXACT_MATCH_ADDR_PKT:
                    pkt_len = TRACE_SHORT_PKT_LEN;
                    break;
                case EXCEPTION_SHORT_ADDR_PKT:
                    pkt_len = EXCEPTION_SHORT_ADDR_PKT_LEN;
                    if (!(trace_bytes[byte_index + (pkt_len - 2)] & CONTINUITY_BIT_MASK))
                        pkt_len = pkt_len - 1;
                    break;
                case EXCEPTION_32BIT_ADDR_PKT:
                    pkt_len = EXCEPTION_32_ADDR_PKT_LEN;
                    break;
                case EXCEPTION_64BIT_ADDR_PKT:
                    pkt_len = EXCEPTION_64_ADDR_PKT_LEN;
                    break;
                }
            }
            break;

        case TRACE_INSTRUMENT_PKT:
            pkt_len = TRACE_INSTRUMENT_PKT_LEN;
            break;

        case TRACE_CC_F2_0_SMALL_COMMIT_PKT:
        case TRACE_CC_F2_1_PKT:
            pkt_len = TRACE_CC_F2_PKT_LEN;
            break;

        case TRACE_CC_F1_X_COUNT_PKT:
            pkt_len = 0;

            /* Cycle Count Format 1_1 - (COMMOPT[29] == 0x1, TRCCCI[7] == 0x1) */
            if (VAL_EXTRACT_BITS(trcidr0_read, 29, 29) == 0x1)
                 pkt_len = 1 + trace_cbit_len(trace_bytes, byte_index, 1, 3);

            /* Cycle Count Format 1_0 - (COMMOPT[29] == 0x0, TRCCCI[7] == 0x1) */
            else {
                 pkt_len = 1 + trace_cbit_len(trace_bytes, byte_index, 1, 5);
                 pkt_len = pkt_len + trace_cbit_len(trace_bytes,
                                      byte_index, pkt_len, 3);
            }
            break;

        case TRACE_CC_F1_X_UNK_COUNT_PKT:
            pkt_len = 0;

            /* Cycle Unknown Count Format 1_1 - (COMMOPT[29] == 0x1, TRCCCI[7] == 0x1) */
            if (VAL_EXTRACT_BITS(trcidr0_read, 29, 29) == 0x1)
                pkt_len = 1;

            /* Cycle Unknown Count Format 1_0 - (COMMOPT[29] == 0x0, TRCCCI[7] == 0x1) */
            else
               pkt_len = 1 + trace_cbit_len(trace_bytes, byte_index, 1, 5);
            break;

        case TRACE_COMMIT_PKT:
            pkt_len = 1 + trace_cbit_len(trace_bytes, byte_index, 1, 5);
            break;

        case TRACE_CONTEXT_PKT:
        case CTX_32BIT_IS0_PKT:
        case CTX_32BIT_IS1_PKT:
        case CTX_64BIT_IS0_PKT:
        case CTX_64BIT_IS1_PKT:
          if (trace_bytes[byte_index] == TRACE_CONTEXT_PKT)
              length = 2;
           if ((trace_bytes[byte_index] == CTX_32BIT_IS0_PKT) ||
               (trace_bytes[byte_index] == CTX_32BIT_IS1_PKT))
              length = 6;
           if ((trace_bytes[byte_index] == CTX_64BIT_IS0_PKT) ||
               (trace_bytes[byte_index] == CTX_64BIT_IS1_PKT))
              length = 10;

            switch (VAL_EXTRACT_BITS(trace_bytes[byte_index + (length - 1)], 6, 7)) {

            case 0: /* Packet Variant 1 */
              pkt_len = length;
              break;
            case 2: /* Packet Variant 2 */
              pkt_len = length + CONTEXTID_LAYOUT_LEN;
              break;
            case 1: /* Packet Variant 3 */
              pkt_len = length + VMID_LAYOUT_LEN;
              break;
            case 3: /* Packet Variant 4 */
              pkt_len = length + CONTEXTID_LAYOUT_LEN + VMID_LAYOUT_LEN;
              break;
            }

           break;
        case TARGET_ADDR_SHORT_IS0_PKT:
        case TARGET_ADDR_SHORT_IS1_PKT:
            pkt_len = TRACE_SHORT_PKT_LEN;
            if (!(trace_bytes[byte_index + (pkt_len - 2)] & CONTINUITY_BIT_MASK))
                            pkt_len = pkt_len - 1;
            break;

        case TARGET_ADDR_32BIT_IS0_PKT:
        case TARGET_ADDR_32BIT_IS1_PKT:
            pkt_len = TARGET_ADDR_32BIT_ISX_PKT_LEN;
            break;
        case TARGET_ADDR_64BIT_IS0_PKT:
        case TARGET_ADDR_64BIT_IS1_PKT:
            pkt_len = TARGET_ADDR_64BIT_ISX_PKT_LEN;
            break;
        case Q_SHORT_ADDR_IS0_PKT:
        case Q_SHORT_ADDR_IS1_PKT:
            pkt_len = TRACE_SHORT_PKT_LEN;
            if (!(trace_bytes[byte_index + (pkt_len - 2)] & CONTINUITY_BIT_MASK))
                            pkt_len = pkt_len - 1;
            pkt_len = pkt_len + trace_cbit_len(trace_bytes, byte_index, pkt_len, 5);
            break;

        case Q_32BIT_ADDR_IS0_PKT:
        case Q_32BIT_ADDR_IS1_PKT:
            pkt_len = Q_32BIT_ADDR_IS0_PKT_A_LEN + trace_cbit_len(trace_bytes,
                                                byte_index, Q_32BIT_ADDR_IS0_PKT_A_LEN, 5);
            break;

        case Q_COUNT_PKT:
            pkt_len = 1 + trace_cbit_len(trace_bytes, byte_index, 1, 5);
            break;

        case SRC_SHORT_ADDR_IS0_PKT:
        case SRC_SHORT_ADDR_IS1_PKT:
            pkt_len = TRACE_SHORT_PKT_LEN;
            if (!(trace_bytes[byte_index + (pkt_len - 2)] & CONTINUITY_BIT_MASK))
                            pkt_len = pkt_len - 1;
            break;

        case SRC_32BIT_ADDR_IS0_PKT:
        case SRC_32BIT_ADDR_IS1_PKT:
              pkt_len = SRC_32BIT_ADDR_PKT_LEN;
              break;

        case SRC_64BIT_ADDR_IS0_PKT:
        case SRC_64BIT_ADDR_IS1_PKT:
              pkt_len = SRC_64BIT_ADDR_PKT_LEN;
              break;

        default:
              uint64_t header = trace_bytes[byte_index];

              /* Handles Cycle Count, Mispredict, Cancel, Event, Atom,
                 and Address Match packet types */
              if (((header & CYCLE_COUNT_FORMAT_3_1_MASK) == CYCLE_COUNT_FORMAT_3_1_VAL) ||
                  ((header & CYCLE_COUNT_FORMAT_3_0_MASK) == CYCLE_COUNT_FORMAT_3_0_VAL) ||
                  ((header & MISPREDICT_PKT_MASK) == MISPREDICT_PKT_VAL) ||
                  ((header & CANCEL_FORMAT_2_PKT_MASK) == CANCEL_FORMAT_2_PKT_VAL) ||
                  ((header & CANCEL_FORMAT_3_PKT_MASK) == CANCEL_FORMAT_3_PKT_VAL) ||
                  ((header & EVENT_PKT_MASK) == EVENT_PKT_VAL) ||
                  ((header & TARGET_ADDR_EXACT_MATCH_MASK) == TARGET_ADDR_EXACT_MATCH_VAL) ||
                  ((header & SRC_ADDR_EXACT_MATCH_MASK) == SRC_ADDR_EXACT_MATCH_VAL) ||
                  ((header & ATOM_FORMAT_X_PKT_MASK) == ATOM_FORMAT_X_PKT_VAL))
                      pkt_len = TRACE_PKT_MIN_LEN;

              /* Handles Cancel Format 1 and Q Exact Match packets */
              else if (((header & CANCEL_FORMAT_1_PKT_MASK) == CANCEL_FORMAT_1_PKT_VAL) ||
                       ((header & Q_EXACT_MATCH_PKT_MASK) == Q_EXACT_MATCH_PKT_VAL))
                pkt_len = 1 + trace_cbit_len(trace_bytes, byte_index, 1, 5);
              else {
                  val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Reserved or Invalid Trace Packet",
                                                                             0, pe_index);
                  return TRACE_PKT_INVALID;
              }
             break;
      }
      byte_index += pkt_len;
  }
  return TRACE_PKT_INVALID;
}



uint64_t val_ete_get_trace_timestamp(uint64_t buffer_address)
{

  uint8_t trace_bytes[100];
  uint32_t i = 0;
  uint64_t timestamp = 0;
  uint64_t ts_value = 0;
  uint32_t ts_start_byte = 0, ts_continue = 0;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());


  val_memcpy(trace_bytes, (void *)buffer_address, 100);
  ts_start_byte = parse_tracestream(trace_bytes, sizeof(trace_bytes));
  if (ts_start_byte == TRACE_PKT_INVALID) {
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n      ETE Parsing failed", 0, index);
    return 0;
  }

  if ((trace_bytes[ts_start_byte] == TRACE_TIMESTAMP_V1_PKT) ||
      (trace_bytes[ts_start_byte] == TRACE_TIMESTAMP_V2_PKT)) {
      ts_continue = 1;
      ts_start_byte++;
      i = 0;
      while (ts_continue && (i < 9)) {
        ts_value = (trace_bytes[ts_start_byte + i] & TS_VALUE_MASK);
        timestamp = timestamp | ((ts_value << (i * 8)) >> i);
        ts_continue = (trace_bytes[ts_start_byte + i] & CONTINUITY_BIT_MASK);
        i++;
      }
  }

  if (timestamp == 0) {
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Timestamp Parsing failed", 0, index);
    return 0;
  }
  else
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       Timestamp Value: 0x%llx", timestamp, index);

  return timestamp;
}

uint64_t val_ete_generate_trace(uint64_t buffer_addr, uint32_t self_hosted_trace_enabled)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint64_t trbptr_before = 0;
    uint64_t trbptr_after = 0;

    /* If SelfHostedTraceEnabled = FALSE, Enable TFO */
    if (self_hosted_trace_enabled == SH_TRACE_ENABLE_FALSE)
        AA64EnableTFO();

    /* Update MDCR_EL2.E2TB to allow Tracing at EL2 */
    AA64SetupTraceAccess();

    /* Update TRBLIMITR_EL1_E/XE Based on SelfHostedTraceEnabled & Enable Trace Buffer Unit */
    if (self_hosted_trace_enabled == SH_TRACE_ENABLE_TRUE)
        AA64EnableTRBUTrace(index, buffer_addr, ACS_TRBLIMITR_EL1_E);
    else
        AA64EnableTRBUTrace(index, buffer_addr, ACS_TRBLIMITR_EL1_XE | ACS_TRBLIMITR_EL1_nVM);

    AA64EnableETETrace();

    /* Read TRBPTR_EL1 before generating the trace */
    trbptr_before = AA64ReadTrbPtrEl1();

    /* Generate Trace */
    AA64GenerateETETrace();

    /* Disable Trace */
    AA64DisableETETrace();
    AA64DisableTRBUTrace();

    /* If SelfHostedTraceEnabled = FALSE, Disable TFO */
    if (self_hosted_trace_enabled == SH_TRACE_ENABLE_FALSE)
        AA64DisableTFO();

    /* Read TRBPTR_EL1 after generating the trace */
    trbptr_after = AA64ReadTrbPtrEl1();

    /* If Trace is not generated or timestamp for current PE not updated */
    if (trbptr_before == trbptr_after)
        return ACS_STATUS_FAIL;

    return val_ete_get_trace_timestamp(buffer_addr + (index << 12));
}
