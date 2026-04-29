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

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>

#include "Include/IndustryStandard/Acpi61.h"
#include "pal_uefi.h"

UINT64 pal_get_fadt_ptr(VOID);

/*
 * DSDT/SSDT AML parser with generic AML helpers.
 *
 * Flow:
 * - Walk AML bytecode linearly and track DeviceOp/ScopeOp scopes using a stack.
 * - When a DeviceOp contains a matching HID/CID and a valid _UID, record it.
 * - Parse DSDT (and SSDTs via XSDT) once and populate while counting.
 *
 * MPAM-specific matching is isolated to pal_mpam_match_hid_cid() and the
 * record callback pal_mpam_record_msc(). All other helpers are AML-generic.
 *
 * This parser supports only the AML constructs needed for MSC discovery:
 * DeviceOp, ScopeOp, NameOp, and simple data objects (string/integer/package).
 */

typedef UINT32 (*PAL_ACPI_MATCH_FN)(PAL_AML_DATA_TYPE data_type,
                                    UINT64 data_value,
                                    CONST CHAR8 *data_text);

typedef VOID (*PAL_ACPI_RECORD_FN)(VOID *context,
                                   UINT32 uid,
                                   CONST CHAR8 *dev_name);

STATIC UINT32
pal_acpi_parse_pkg_length(CONST UINT8 * data,
                          UINT32 length,
                          UINT32 *pkg_length,
                          UINT32 *consumed);

STATIC UINT32
pal_acpi_parse_data_object(CONST UINT8 * data,
                           UINT32 length,
                           PAL_AML_DATA_TYPE *type,
                           UINT64 *value,
                           CHAR8 *text,
                           UINT32 text_len,
                           UINT32 *consumed);

typedef struct {
  UINT32 end_offset;
  UINT32 has_match;
  UINT32 uid_valid;
  UINT32 uid;
  CHAR8  name_seg[5];
} PAL_ACPI_DEVICE_SCOPE;

/**
  @brief Compare two ASCII strings for equality.
**/
STATIC UINT32
pal_acpi_string_equal(CONST CHAR8 *left, CONST CHAR8 *right)
{
  UINT32 idx = 0u;
  if ((left == NULL) || (right == NULL))
    return 0u;
  while (left[idx] != '\0' || right[idx] != '\0') {
    if (left[idx] != right[idx])
      return 0u;
    idx++;
  }
  return 1u;
}

/**
  @brief Decode EISA ID encoded in integer _HID into 7-char string.
**/
STATIC VOID
pal_acpi_decode_eisa_id(UINT32 encoded, CHAR8 *out, UINT32 out_len)
{
  if ((out == NULL) || (out_len < 8u))
    return;
  out[0] = (CHAR8)(((encoded >> 26) & 0x1Fu) + 'A' - 1);
  out[1] = (CHAR8)(((encoded >> 21) & 0x1Fu) + 'A' - 1);
  out[2] = (CHAR8)(((encoded >> 16) & 0x1Fu) + 'A' - 1);
  UINT16 product = (UINT16)(encoded & 0xFFFFu);
  for (UINT32 idx = 0; idx < 4u; idx++) {
    UINT8 nibble = (UINT8)((product >> ((3u - idx) * 4u)) & 0xFu);
    out[3u + idx] = (CHAR8)(nibble < 10u ? ('0' + nibble) : ('A' + nibble - 10u));
  }
  out[7] = '\0';
}

/**
  @brief MPAM-specific HID/CID matcher for ARMHAA5C.
**/
STATIC UINT32
pal_mpam_match_hid_cid(PAL_AML_DATA_TYPE data_type,
                       UINT64 data_value,
                       CONST CHAR8 *data_text)
{
  /* Accepts either string HID/CID or EISA-encoded integer forms. */
  if ((data_type == AML_DATA_STRING) &&
      pal_acpi_string_equal(data_text, "ARMHAA5C")) {
    return 1u;
  }
  if (data_type == AML_DATA_INTEGER) {
    CHAR8 decoded[8];
    SetMem(decoded, sizeof(decoded), 0);
    pal_acpi_decode_eisa_id((UINT32)data_value, decoded, sizeof(decoded));
    if (pal_acpi_string_equal(decoded, "ARMHAA5C"))
      return 1u;
  }
  return 0u;
}

/**
  @brief Parse a Package() object and match any HID/CID entry.
**/
STATIC UINT32
pal_acpi_parse_hid_cid_package(CONST UINT8 *data,
                               UINT32 length,
                               PAL_ACPI_MATCH_FN match_fn)
{
  /* Parse Package() elements and match any HID/CID entry via the MPAM matcher. */
  UINT32 pkg_length = 0u;
  UINT32 consumed = 0u;
  UINT32 offset = 0u;
  UINT32 end = 0u;
  if ((data == NULL) || (length == 0u) || (data[0] != AML_OP_PACKAGE))
    return 0u;
  if (!pal_acpi_parse_pkg_length(&data[1], length - 1u, &pkg_length, &consumed))
    return 0u;
  offset = 1u + consumed;
  end = offset + pkg_length;
  if (end > length || offset >= length)
    return 0u;
  /* Package element count is one byte */
  offset += 1u;
  while (offset < end) {
    PAL_AML_DATA_TYPE data_type;
    UINT64 data_value = 0u;
    CHAR8 data_text[16];
    UINT32 data_consumed = 0u;
    SetMem(data_text, sizeof(data_text), 0);
    if (!pal_acpi_parse_data_object(&data[offset], end - offset, &data_type,
                                    &data_value, data_text, sizeof(data_text),
                                    &data_consumed)) {
      break;
    }
    if ((match_fn != NULL) && match_fn(data_type, data_value, data_text))
      return 1u;
    offset += data_consumed;
  }
  return 0u;
}

/**
  @brief Decode AML PkgLength field and return length/consumed bytes.
**/
STATIC UINT32
pal_acpi_parse_pkg_length(CONST UINT8 *data,
                          UINT32 length,
                          UINT32 *pkg_length,
                          UINT32 *consumed)
{
  /* Decode AML PkgLength encoding (1-4 bytes, high bits indicate byte count). */
  UINT8 lead;
  UINT32 byte_count;
  UINT32 value;
  if ((data == NULL) || (pkg_length == NULL) || (consumed == NULL) || (length == 0u))
    return 0u;
  lead = data[0];
  byte_count = (UINT32)(lead >> 6);
  if (byte_count > 3u)
    return 0u;
  if ((1u + byte_count) > length)
    return 0u;
  if (byte_count == 0u)
    value = lead & 0x3Fu;
  else
    value = lead & 0x0Fu;
  for (UINT32 idx = 0; idx < byte_count; idx++)
    value |= (UINT32)data[1u + idx] << (4u + (idx * 8u));
  *pkg_length = value;
  *consumed = 1u + byte_count;
  return 1u;
}

/**
  @brief Parse AML NameString and return the final NameSeg.
**/
STATIC UINT32
pal_acpi_parse_name_string(CONST UINT8 *data, UINT32 length, CHAR8 name[5])
{
  /* Supports: Root/Parent prefixes, Dual/Multiname prefixes, and NameSeg. */
  UINT32 offset = 0u;
  if ((data == NULL) || (length == 0u))
    return 0u;
  while ((offset < length) &&
         ((data[offset] == AML_NAME_ROOT) || (data[offset] == AML_NAME_PARENT)))
    offset++;
  if (offset >= length)
    return 0u;
  if (data[offset] == AML_NAME_NULL) {
    if (name != NULL)
      name[0] = '\0';
    return offset + 1u;
  }
  if (data[offset] == AML_NAME_DUAL) {
    if ((offset + 1u + 8u) > length)
      return 0u;
    if (name != NULL) {
      CopyMem(name, &data[offset + 1u + 4u], 4u);
      name[4] = '\0';
    }
    return offset + 1u + 8u;
  }
  if (data[offset] == AML_NAME_MULTI) {
    UINT32 count;
    if ((offset + 1u) >= length)
      return 0u;
    count = data[offset + 1u];
    if ((offset + 2u + (count * 4u)) > length)
      return 0u;
    if ((name != NULL) && (count > 0u)) {
      UINT32 last = offset + 2u + ((count - 1u) * 4u);
      CopyMem(name, &data[last], 4u);
      name[4] = '\0';
    }
    return offset + 2u + (count * 4u);
  }
  if ((offset + 4u) > length)
    return 0u;
  if (name != NULL) {
    CopyMem(name, &data[offset], 4u);
    name[4] = '\0';
  }
  return offset + 4u;
}

/**
  @brief Parse AML data objects used by _HID/_CID/_UID.
**/
STATIC UINT32
pal_acpi_parse_data_object(CONST UINT8 *data,
                           UINT32 length,
                           PAL_AML_DATA_TYPE *type,
                           UINT64 *value,
                           CHAR8 *text,
                           UINT32 text_len,
                           UINT32 *consumed)
{
  /* Supports Integer, String, and Package used by _HID/_CID/_UID. */
  UINT8 op;
  if ((data == NULL) || (length == 0u) || (type == NULL) || (consumed == NULL))
    return 0u;
  op = data[0];
  *type = AML_DATA_NONE;
  *consumed = 0u;
  switch (op) {
  case AML_OP_ZERO:
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = 0u;
    *consumed = 1u;
    return 1u;
  case AML_OP_ONE:
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = 1u;
    *consumed = 1u;
    return 1u;
  case AML_OP_BYTE:
    if (length < 2u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = data[1];
    *consumed = 2u;
    return 1u;
  case AML_OP_WORD:
    if (length < 3u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = (UINT16)(data[1] | (data[2] << 8));
    *consumed = 3u;
    return 1u;
  case AML_OP_DWORD:
    if (length < 5u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL)
      *value = (UINT32)(data[1] | (data[2] << 8) |
                        (data[3] << 16) | (data[4] << 24));
    *consumed = 5u;
    return 1u;
  case AML_OP_QWORD:
    if (length < 9u)
      return 0u;
    *type = AML_DATA_INTEGER;
    if (value != NULL) {
      *value = ((UINT64)data[1]) |
               ((UINT64)data[2] << 8) |
               ((UINT64)data[3] << 16) |
               ((UINT64)data[4] << 24) |
               ((UINT64)data[5] << 32) |
               ((UINT64)data[6] << 40) |
               ((UINT64)data[7] << 48) |
               ((UINT64)data[8] << 56);
    }
    *consumed = 9u;
    return 1u;
  case AML_OP_STRING: {
    UINT32 text_index = 0u;
    *type = AML_DATA_STRING;
    if (text != NULL)
      text[0] = '\0';
    for (UINT32 idx = 1u; idx < length; idx++) {
      if (data[idx] == '\0') {
        if ((text != NULL) && (text_len > 0u))
          text[text_index] = '\0';
        *consumed = idx + 1u;
        return 1u;
      }
      if ((text != NULL) && (text_len > 1u) && (text_index < text_len - 1u)) {
        text[text_index] = (CHAR8)data[idx];
        text_index++;
      }
    }
    return 0u;
  }
  default:
    return 0u;
  }
}

/**
  @brief Parse numeric strings like "10" or "0x10" for _UID.
**/
STATIC UINT32
pal_acpi_parse_numeric_string(CONST CHAR8 *text, UINT32 *value_out)
{
  /* Parse numeric strings like "10" or "0x10" used by _UID. */
  UINT32 value = 0u;
  UINT32 base = 10u;
  UINT32 idx = 0u;
  if ((text == NULL) || (value_out == NULL) || (text[0] == '\0'))
    return 0u;
  if ((text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X'))) {
    base = 16u;
    idx = 2u;
  }
  for (; text[idx] != '\0'; idx++) {
    UINT32 digit;
    if ((text[idx] >= '0') && (text[idx] <= '9'))
      digit = (UINT32)(text[idx] - '0');
    else if ((base == 16u) && (text[idx] >= 'a') && (text[idx] <= 'f'))
      digit = (UINT32)(text[idx] - 'a') + 10u;
    else if ((base == 16u) && (text[idx] >= 'A') && (text[idx] <= 'F'))
      digit = (UINT32)(text[idx] - 'A') + 10u;
    else
      return 0u;
    value = (value * base) + digit;
  }
  *value_out = value;
  return 1u;
}

/**
  @brief Copy NameSeg into a C string, trimming trailing '_' padding.
**/
STATIC VOID
pal_acpi_copy_name_seg(CHAR8 *out, UINT32 out_len, CONST CHAR8 name_seg[5])
{
  /* Trim trailing '_' padding in 4-char NameSegs. */
  UINT32 seg_len = 0u;
  if ((out == NULL) || (out_len == 0u) || (name_seg == NULL))
    return;
  while (seg_len < 4u && name_seg[seg_len] != '\0')
    seg_len++;
  while (seg_len > 1u && name_seg[seg_len - 1u] == '_')
    seg_len--;
  if (seg_len >= out_len)
    seg_len = out_len - 1u;
  CopyMem(out, name_seg, seg_len);
  out[seg_len] = '\0';
}

/**
  @brief MPAM record callback: update matching MSC entry with name.
**/
STATIC VOID
pal_mpam_record_msc(VOID *context,
                    UINT32 uid,
                    CONST CHAR8 *dev_name)
{
  /* Find matching MSC by identifier and update its device object name. */
  MPAM_INFO_TABLE *table = (MPAM_INFO_TABLE *)context;
  if ((table == NULL) || (dev_name == NULL))
    return;
  MPAM_MSC_NODE *entry = &table->msc_node[0];
  for (UINT32 idx = 0; idx < table->msc_count; idx++) {
    if (entry->identifier == uid) {
      AsciiStrnCpyS(entry->device_obj_name, MAX_NAMED_COMP_LENGTH,
                    dev_name, (MAX_NAMED_COMP_LENGTH - 1));
      return;
    }
    entry = MPAM_NEXT_MSC(entry);
  }
}

/**
  @brief Generic AML walker: track scopes, match HID/CID, record on UID.
**/
STATIC UINT32
pal_acpi_parse_aml_for_device(CONST UINT8 *aml,
                              UINT32 length,
                              PAL_ACPI_MATCH_FN match_fn,
                              PAL_ACPI_RECORD_FN record_fn,
                              VOID *record_ctx)
{
  /*
   * Walk AML and track nested DeviceOp/ScopeOp blocks.
   * Each stack entry keeps: end_offset, name segment, HID/CID match state, UID.
   * On scope exit, if match + UID is present, record the MSC entry.
   * This records only the DeviceOp name segment (e.g., "MSC0").
   */
  PAL_ACPI_DEVICE_SCOPE stack[PAL_AML_MAX_DEVICE_DEPTH];
  INT32 depth = -1;
  UINT32 offset = 0u;
  UINT32 msc_count = 0u;
  while (offset < length) {
    /* Pop completed scopes before processing the next opcode. */
    while ((depth >= 0) && (offset >= stack[depth].end_offset)) {
      if ((stack[depth].has_match != 0u) && (stack[depth].uid_valid != 0u)) {
        CHAR8 dev_name[MAX_NAMED_COMP_LENGTH];
        SetMem(dev_name, sizeof(dev_name), 0);
        pal_acpi_copy_name_seg(dev_name, sizeof(dev_name), stack[depth].name_seg);
        if (record_fn != NULL) {
          pal_print_msg(ACS_PRINT_INFO,
                        " DSDT MSC: UID=0x%x Device=%a\n",
                        stack[depth].uid,
                        dev_name);
          record_fn(record_ctx, stack[depth].uid, dev_name);
        }
        msc_count++;
      }
      depth--;
    }
    /* DeviceOp: enter a new device scope and record its end offset and NameSeg. */
    if ((offset + 1u) < length &&
        (aml[offset] == AML_OP_DEVICE_PREFIX) &&
        (aml[offset + 1u] == AML_OP_DEVICE)) {
      UINT32 pkg_length;
      UINT32 pkg_consumed;
      UINT32 name_consumed;
      UINT32 body_start;
      UINT32 end_offset;
      offset += 2u;
      if (!pal_acpi_parse_pkg_length(&aml[offset], length - offset,
                                     &pkg_length, &pkg_consumed))
      {
        /* Debug: malformed DeviceOp, skip */
        offset++;
        continue;
      }
      body_start = offset + pkg_consumed;
      end_offset = offset + pkg_length;
      if (end_offset > length) {
        /* Debug: malformed DeviceOp, skip */
        offset++;
        continue;
      }
      if (depth + 1 >= (INT32)PAL_AML_MAX_DEVICE_DEPTH) {
        offset = end_offset;
        continue;
      }
      depth++;
      SetMem(&stack[depth], sizeof(stack[depth]), 0);
      stack[depth].end_offset = end_offset;
      name_consumed =
        pal_acpi_parse_name_string(&aml[body_start], end_offset - body_start,
                                   stack[depth].name_seg);
      if (name_consumed == 0u) {
        offset = end_offset;
        continue;
      }
      offset = body_start + name_consumed;
      continue;
    }
    /* ScopeOp: enter a new named scope (used for nesting). */
    if (aml[offset] == AML_OP_SCOPE) {
      UINT32 pkg_length;
      UINT32 pkg_consumed;
      UINT32 name_consumed;
      UINT32 body_start;
      UINT32 end_offset;
      offset += 1u;
      if (!pal_acpi_parse_pkg_length(&aml[offset], length - offset,
                                     &pkg_length, &pkg_consumed)) {
        /* Debug: malformed ScopeOp, skip */
        offset++;
        continue;
      }
      body_start = offset + pkg_consumed;
      end_offset = offset + pkg_length;
      if (end_offset > length) {
        /* Debug: malformed ScopeOp, skip */
        offset++;
        continue;
      }
      if (depth + 1 >= (INT32)PAL_AML_MAX_DEVICE_DEPTH) {
        offset = end_offset;
        continue;
      }
      depth++;
      SetMem(&stack[depth], sizeof(stack[depth]), 0);
      stack[depth].end_offset = end_offset;
      name_consumed =
        pal_acpi_parse_name_string(&aml[body_start], end_offset - body_start,
                                   stack[depth].name_seg);
      if (name_consumed == 0u) {
        offset = end_offset;
        continue;
      }
      offset = body_start + name_consumed;
      continue;
    }
    /* NameOp: parse _HID/_CID/_UID and their data objects. */
    if (aml[offset] == AML_OP_NAME) {
      CHAR8 name[5];
      UINT32 name_consumed;
      UINT32 data_consumed;
      PAL_AML_DATA_TYPE data_type;
      UINT64 data_value = 0u;
      CHAR8 data_text[16];
      offset += 1u;
      name_consumed = pal_acpi_parse_name_string(&aml[offset], length - offset, name);
      if (name_consumed == 0u) {
        offset++;
        continue;
      }
      offset += name_consumed;
      SetMem(data_text, sizeof(data_text), 0);
      if (!pal_acpi_parse_data_object(&aml[offset], length - offset, &data_type,
                                      &data_value, data_text, sizeof(data_text),
                                      &data_consumed)) {
        /* _CID can be a Package of multiple IDs; parse and match if so. */
        if ((pal_acpi_string_equal(name, "_HID") ||
             pal_acpi_string_equal(name, "_CID")) &&
            (aml[offset] == AML_OP_PACKAGE)) {
          UINT32 pkg_length = 0u;
          UINT32 pkg_consumed = 0u;
          if (pal_acpi_parse_hid_cid_package(&aml[offset], length - offset,
                                             match_fn)) {
            stack[depth].has_match = 1u;
          }
          if (pal_acpi_parse_pkg_length(&aml[offset + 1u], length - offset - 1u,
                                        &pkg_length, &pkg_consumed)) {
            offset += 1u + pkg_consumed + pkg_length;
            continue;
          }
        }
        offset++;
        continue;
      }
      if (depth >= 0) {
        if (pal_acpi_string_equal(name, "_HID") ||
            pal_acpi_string_equal(name, "_CID")) {
          /* Match either a string HID/CID or an EISA-encoded integer. */
          if ((match_fn != NULL) &&
              match_fn(data_type, data_value, data_text)) {
            stack[depth].has_match = 1u;
          } else if (pal_acpi_parse_hid_cid_package(&aml[offset], length - offset,
                                                    match_fn)) {
            stack[depth].has_match = 1u;
          }
        } else if (pal_acpi_string_equal(name, "_UID")) {
          /* _UID can be integer or numeric string. */
          UINT32 uid_value;
          if (data_type == AML_DATA_INTEGER) {
            stack[depth].uid = (UINT32)data_value;
            stack[depth].uid_valid = 1u;
          } else if ((data_type == AML_DATA_STRING) &&
                     pal_acpi_parse_numeric_string(data_text, &uid_value)) {
            stack[depth].uid = uid_value;
            stack[depth].uid_valid = 1u;
          }
        }
      }
      offset += data_consumed;
      continue;
    }
    offset++;
  }
  /* Flush any remaining scopes at end of AML. */
  while (depth >= 0) {
    if ((stack[depth].has_match != 0u) && (stack[depth].uid_valid != 0u)) {
      CHAR8 dev_name[MAX_NAMED_COMP_LENGTH];
      SetMem(dev_name, sizeof(dev_name), 0);
      pal_acpi_copy_name_seg(dev_name, sizeof(dev_name), stack[depth].name_seg);
      if (record_fn != NULL) {
        pal_print_msg(ACS_PRINT_INFO,
                      " DSDT MSC: UID=0x%x Device=%a\n",
                      stack[depth].uid,
                      dev_name);
        record_fn(record_ctx, stack[depth].uid, dev_name);
      }
      msc_count++;
    }
    depth--;
  }
  return msc_count;
}

/**
  @brief Parse DSDT/SSDT and populate MPAM MSC device object names.
**/
UINT32
pal_mpam_parse_dsdt_info(MPAM_INFO_TABLE *MpamTable)
{
  /*
   * Resolve DSDT from FADT, count MSC entries while populating the table.
   * SSDTs referenced by XSDT are parsed in the same pass as well.
   */
  EFI_ACPI_DESCRIPTION_HEADER *dsdt;
  EFI_ACPI_DESCRIPTION_HEADER *xsdt;
  EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE *fadt_table;
  UINT64 fadt;
  UINT64 *entries;
  UINT32 entry_count;
  UINT32 msc_count = 0u;

  /* Locate XSDT and FADT to find DSDT pointer. */
  xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)pal_get_xsdt_ptr();
  if (xsdt == NULL) {
    return 0u;
  }

  fadt = pal_get_fadt_ptr();
  if (fadt == 0u)
    return 0u;

  fadt_table = (EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE *)(UINTN)fadt;
  /* Prefer XDsdt (64-bit); fall back to legacy Dsdt. */
  if (fadt_table->XDsdt != 0u)
    dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)fadt_table->XDsdt;
  else if (fadt_table->Dsdt != 0u)
    dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)fadt_table->Dsdt;
  else
    return 0u;

  /* AML payload starts after the ACPI header. */
  if (dsdt->Length <= sizeof(*dsdt))
    return 0u;

  /* Single pass: count and populate (if table is provided). */
  msc_count = pal_acpi_parse_aml_for_device((CONST UINT8 *)(dsdt + 1),
                                            dsdt->Length - sizeof(*dsdt),
                                            pal_mpam_match_hid_cid,
                                            pal_mpam_record_msc,
                                            MpamTable);
  if (xsdt->Length >= sizeof(*xsdt)) {
    entry_count = (xsdt->Length - sizeof(*xsdt)) / sizeof(UINT64);
    entries = (UINT64 *)(xsdt + 1);
    for (UINT32 idx = 0; idx < entry_count; idx++) {
      EFI_ACPI_DESCRIPTION_HEADER *table =
        (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)entries[idx];
      if ((table == NULL) ||
          (table->Signature != EFI_ACPI_6_1_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
        continue;
      if (table->Length <= sizeof(*table))
        continue;
      msc_count += pal_acpi_parse_aml_for_device((CONST UINT8 *)(table + 1),
                                                 table->Length - sizeof(*table),
                                                 pal_mpam_match_hid_cid,
                                                 pal_mpam_record_msc,
                                                 MpamTable);
    }
  }
  return msc_count;
}
