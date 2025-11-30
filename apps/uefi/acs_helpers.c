/** @file
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
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

#include <Uefi.h>
#include <stdbool.h>
#include <Library/UefiLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ShellParameters.h>

#include "val/include/val_interface.h"
#include "val/include/acs_pe.h"
#include "val/include/acs_val.h"
#include "val/include/acs_memory.h"
#include "val/include/rule_based_execution.h"
#include "acs.h"

/* Minimal ASCII string equality check to avoid extra deps */
static BOOLEAN ascii_streq(const CHAR8 *a, const CHAR8 *b)
{
    if (a == NULL || b == NULL) return FALSE;
    while (*a && *b) {
        if (*a != *b) return FALSE;
        a++; b++;
    }
    return (*a == '\0' && *b == '\0');
}

/* Case-insensitive ASCII compare for CHAR16 literals (only A-Z handled) */
static BOOLEAN w_ascii_streq_caseins(const CHAR16 *a, const CHAR16 *b)
{
    CHAR16 ca;
    CHAR16 cb;
    if (a == NULL || b == NULL) return FALSE;
    while (*a && *b) {
        ca = *a;
        cb = *b;
        if (ca >= L'A' && ca <= L'Z') ca = (CHAR16)(ca - L'A' + L'a');
        if (cb >= L'A' && cb <= L'Z') cb = (CHAR16)(cb - L'A' + L'a');
        if (ca != cb) return FALSE;
        a++; b++;
    }
    return (*a == L'\0' && *b == L'\0');
}

/* Map a wide token to MODULE_NAME_e and append to list if capacity allows.
        Accepts only module names that match module_name_string[] (no numeric IDs). */
static VOID map_and_add_module_token(
    const CHAR16 *wtoken,
    UINT32 *list,
    UINT32 *count,
    UINT32 capacity
)
{
    const CHAR16 *start;
    const CHAR16 *end;
    CHAR16 wbuf[32];
    UINTN tlen;
    UINTN i;
    CHAR8 abuf[32];
    UINT32 mid;
    if (wtoken == NULL) return;
    /* trim leading/trailing whitespace including CR/LF */
    start = wtoken;
    while (*start == L' ' || *start == L'\t' || *start == L'\r' || *start == L'\n') start++;
    end = start;
    while (*end)
        end++;
    while (end > start &&
                    (end[-1] == L' ' || end[-1] == L'\t' || end[-1] == L'\r' || end[-1] == L'\n'))
        end--;
    if (end <= start) return;

    /* Copy token into bounded buffer */
    tlen = (UINTN)(end - start);
    if (tlen >= (sizeof(wbuf)/sizeof(wbuf[0]))) tlen = (sizeof(wbuf)/sizeof(wbuf[0])) - 1;
    for (i = 0; i < tlen; i++) wbuf[i] = start[i];
    wbuf[tlen] = L'\0';

    /* Convert to ASCII and compare to module_name_string */
    i = 0;
    for (; i < tlen && i < (sizeof(abuf) - 1); i++)
        abuf[i] = (CHAR8)(wbuf[i] & 0xFF);
    abuf[i] = '\0';

    for (mid = 0; mid < MODULE_ID_SENTINEL; mid++) {
        if (module_name_string[mid] &&
                ascii_streq((CHAR8 *)module_name_string[mid], abuf)) {
            if (*count < capacity) list[(*count)++] = mid;
            return;
        }
    }

    Print(L"Invalid module: %s\n", wbuf);
    g_invalid_arg_seen = TRUE;
}

/* Map a wide token to RULE_ID_e and append to list if capacity allows */
static VOID map_and_add_rule_token(
    const CHAR16 *wtoken,
    RULE_ID_e *list,
    UINT32 *count,
    UINT32 capacity
)
{
    const CHAR16 *start;
    const CHAR16 *end;
    CHAR16 wbuf[64];
    UINTN tlen;
    CHAR8 abuf[64];
    UINTN i;
    UINT32 rid;
    /* trim leading/trailing spaces and tabs */
    if (wtoken == NULL) return;
    start = wtoken;
    while (*start == L' ' || *start == L'\t') start++;
    end = start;
    while (*end)
        end++;
    while (end > start && (end[-1] == L' ' || end[-1] == L'\t'))
        end--;
    if (end <= start) return; /* empty token */

    /* Copy into bounded buffer */
    tlen = (UINTN)(end - start);
    if (tlen >= (sizeof(wbuf)/sizeof(wbuf[0]))) tlen = (sizeof(wbuf)/sizeof(wbuf[0])) - 1;
    for (i = 0; i < tlen; i++) wbuf[i] = start[i];
    wbuf[tlen] = L'\0';

    /* Convert to ASCII for comparison with rule_id_string */
    i = 0;
    for (; i < tlen && i < (sizeof(abuf) - 1); i++)
        abuf[i] = (CHAR8)(wbuf[i] & 0xFF);
    abuf[i] = '\0';

    for (rid = 0; rid < RULE_ID_SENTINEL; rid++) {
        if (rule_id_string[rid] &&
                ascii_streq((CHAR8 *)rule_id_string[rid], abuf)) {
            if (*count < capacity) {
                list[(*count)++] = (RULE_ID_e)rid;
            }
            return;
        }
    }

    /* Not found: print invalid once for visibility */
    Print(L"Invalid rule id: %s\n", wbuf);
    g_invalid_arg_seen = TRUE;
}

/* Try open a file read-only; return TRUE if opened */
static BOOLEAN try_open_readonly(const CHAR16 *Path, SHELL_FILE_HANDLE *Handle)
{
    EFI_STATUS S = ShellOpenFileByName(Path, Handle, EFI_FILE_MODE_READ, 0);
    return (S == EFI_SUCCESS && *Handle != NULL);
}

/* Read entire file into a byte buffer (dynamically grown) */
static EFI_STATUS read_all_bytes(SHELL_FILE_HANDLE Handle, UINT8 **OutBuf, UINTN *OutLen)
{
    const UINTN CHUNK = 1024;
    UINTN cap = 2048;
    UINT8 *buf = NULL;
    EFI_STATUS Status = gBS->AllocatePool(EfiBootServicesData, cap, (VOID **)&buf);
    UINT8 tmp[CHUNK];
    UINTN want;
    EFI_STATUS S;
    UINTN ncap;
    UINT8 *nbuf;
    UINTN i;
    UINTN j;

    if (EFI_ERROR(Status))
        return Status;
    UINTN len = 0;

    /* Reset to start */
    ShellSetFilePosition(Handle, 0);

    for (;;) {
        want = sizeof(tmp);
        S = ShellReadFile(Handle, &want, tmp);

        if (EFI_ERROR(S)) {
            Status = S;
            break;
        }
        if (want == 0) break; /* EOF */
        /* grow if needed */
        if (len + want > cap) {
            ncap = (cap * 3) / 2;
            if (ncap < len + want) ncap = len + want + 512;
            Status = gBS->AllocatePool(EfiBootServicesData, ncap, (VOID **)&nbuf);
            if (EFI_ERROR(Status))
                break;
            for (i = 0; i < len; i++) nbuf[i] = buf[i];
            gBS->FreePool(buf);
            buf = nbuf; cap = ncap;
        }
        for (j = 0; j < want; j++) buf[len + j] = tmp[j];
        len += want;
    }

    if (EFI_ERROR(Status)) {
        if (buf) gBS->FreePool(buf);
        return Status;
    }

    *OutBuf = buf; *OutLen = len;
    return EFI_SUCCESS;
}

/* Convert raw bytes to a temporary CHAR16 buffer, handling ASCII or UTF-16LE */
static EFI_STATUS bytes_to_wchars(const UINT8 *bytes, UINTN blen, CHAR16 **out, UINTN *wlen)
{
    if (bytes == NULL || blen == 0) { *out = NULL; *wlen = 0; return EFI_SUCCESS; }

    BOOLEAN utf16 = FALSE; UINTN offset = 0;
    UINTN zeros = 0;
    UINTN c;
    EFI_STATUS S;
    const UINT8 *p;
    UINTN i;
    if (blen >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) { utf16 = TRUE; offset = 2; }
    else {
        /* heuristic: many zeros suggests UTF-16LE */
        zeros = 0;
        for (i = 0; i < blen; i++) if (bytes[i] == 0) zeros++;
        if (zeros > (blen / 4)) utf16 = TRUE;
    }

    if (utf16) {
        c = (blen - offset) / 2;
        S = gBS->AllocatePool(EfiBootServicesData,
                                         (c + 1) * sizeof(CHAR16),
                                         (VOID **)out);

        if (EFI_ERROR(S))
            return S;
        p = bytes + offset;
        for (i = 0; i < c; i++) {
            (*out)[i] = (CHAR16)(p[2*i] | ((UINT16)p[2*i + 1] << 8));
        }
        (*out)[c] = L'\0'; *wlen = c;
    } else {
        S = gBS->AllocatePool(EfiBootServicesData,
                                         (blen + 1) * sizeof(CHAR16),
                                         (VOID **)out);

        if (EFI_ERROR(S))
            return S;
        for (i = 0; i < blen; i++) (*out)[i] = (CHAR16)bytes[i];
        (*out)[blen] = L'\0'; *wlen = blen;
    }
    return EFI_SUCCESS;
}

/* Count tokens in a rules text (delims: comma/newline; lines starting with # ignored) */
static UINT32 count_rule_tokens_in_text(const CHAR16 *text, UINTN len)
{
    UINT32 count = 0;
    BOOLEAN in_token = FALSE;
    BOOLEAN line_start = TRUE;
    BOOLEAN in_comment = FALSE;
    UINTN i;
    for (i = 0; i < len; i++) {
        CHAR16 ch = text[i];
        if (line_start) {
            /* skip leading spaces/tabs at line start */
            while ((ch == L' ' || ch == L'\t') && (i + 1) < len) ch = text[++i];
            if (ch == L'#') in_comment = TRUE;
            line_start = FALSE;
        }
        if (in_comment) {
            if (ch == L'\n' || ch == L'\r') { in_comment = FALSE; line_start = TRUE; }
            continue;
        }
        if (ch == L',' || ch == L'\n' || ch == L'\r') {
            if (in_token) { count++; in_token = FALSE; }
            if (ch == L'\n' || ch == L'\r') { line_start = TRUE; }
        } else if (ch != L' ' && ch != L'\t') {
            in_token = TRUE;
        }
        if (ch == L'\n' || ch == L'\r') { line_start = TRUE; }
    }
    if (in_token) count++;
    return count;
}

/* Parse a rules text buffer into list using same tokenization */
static VOID
parse_rules_text_into_list(
    const CHAR16 *text,
    UINTN len,
    RULE_ID_e *list,
    UINT32 *count,
    UINT32 capacity)
{
    UINTN i = 0; BOOLEAN line_start = TRUE; BOOLEAN in_comment = FALSE;
    UINTN start;
    UINTN end;
    CHAR16 wtmp[64];
    UINTN tlen;
    UINTN k;
    while (i < len) {
        /* handle line-start comment */
        if (line_start) {
            /* skip leading spaces/tabs */
            while (i < len && (text[i] == L' ' || text[i] == L'\t')) i++;
            if (i < len && text[i] == L'#') {
                in_comment = TRUE;
            }
            line_start = FALSE;
        }

        if (in_comment) {
            while (i < len && text[i] != L'\n' && text[i] != L'\r')
                i++;
            if (i < len && (text[i] == L'\n' || text[i] == L'\r')) {
                line_start = TRUE;
                in_comment = FALSE;
                i++;
            }
            continue;
        }

        /* capture token until delimiter */
        start = i;
        while (i < len && text[i] != L',' && text[i] != L'\n' && text[i] != L'\r')
            i++;
        end = i;

        /* trim */
        while (start < end && (text[start] == L' ' || text[start] == L'\t')) start++;
        while (end > start && (text[end-1] == L' ' || text[end-1] == L'\t'))
            end--;
        if (end > start) {
            tlen = end - start;
            if (tlen >= (sizeof(wtmp)/sizeof(wtmp[0])))
                tlen = (sizeof(wtmp)/sizeof(wtmp[0])) - 1;
            for (k = 0; k < tlen; k++) wtmp[k] = text[start + k];
            wtmp[tlen] = L'\0';
            map_and_add_rule_token(wtmp, list, count, capacity);
        }

        if (i < len && (text[i] == L',' || text[i] == L'\n' || text[i] == L'\r')) {
            if (text[i] == L'\n' || text[i] == L'\r') line_start = TRUE;
            i++;
        }
    }
}

UINT32
command_init (void)
{

    LIST_ENTRY         *ParamPackage;
    CONST CHAR16       *CmdLineArg;
    CHAR16             *ProbParam;
    UINT32             Status;
    UINT32             ReadVerbosity;
    CONST CHAR16       *ArchArg;
    BOOLEAN             has_l;
    BOOLEAN             has_only;
    BOOLEAN             has_fr;
    BOOLEAN             has_hyp;
    BOOLEAN             has_os;
    BOOLEAN             has_ps;
    UINTN               len;
    UINTN               mlen;
    UINTN               k;
    UINTN               start;
    UINTN               end;
    UINTN               tlen;
    UINTN               ti;
    UINTN               ai;
    UINTN               j;
    UINTN               t;
    UINTN               sl;
    UINTN               baseLen;
    UINTN               extra;
    UINTN               alloc;
    UINT32              max_tokens;
    EFI_STATUS          EStatus;
    EFI_STATUS          est;
    EFI_STATUS          RS;
    CHAR16             *Combined;
    UINTN               CombinedLen;
    EFI_SHELL_PARAMETERS_PROTOCOL *ShellParams;
    SHELL_FILE_HANDLE   rfile;
    UINT8              *raw;
    UINTN               raw_len;
    CHAR16             *wtext;
    UINTN               wlen;
    CHAR16              wtoken[64];
    CHAR16             *seg;
    CHAR16              last;

    /* Process Command Line arguments */
    Status = ShellInitialize();
    Status = ShellCommandLineParse (ParamList, &ParamPackage, &ProbParam, TRUE);
    if (Status) {
        Print(L"Shell command line parse error %x\n", Status);
        Print(L"Unrecognized option %s passed\n", ProbParam);
        HelpMsg();
        return SHELL_INVALID_PARAMETER;
    }

    /* Validate -a argument if provided */
    ArchArg = ShellCommandLineGetValue(ParamPackage, L"-a");
    if (ArchArg != NULL) {
        if (!(w_ascii_streq_caseins(ArchArg, L"bsa") ||
                    w_ascii_streq_caseins(ArchArg, L"sbsa")  ||
                    w_ascii_streq_caseins(ArchArg, L"pcbsa") ||
                    w_ascii_streq_caseins(ArchArg, L"vbsa"))) {
            Print(L"Invalid value for -a. Use 'bsa', 'sbsa', 'pcbsa' or 'vbsa'\n");
            HelpMsg();
            return SHELL_INVALID_PARAMETER;
        }
        /* Record arch selection for orchestrator to expand */
        if (w_ascii_streq_caseins(ArchArg, L"bsa"))
            g_arch_selection = ARCH_BSA;
        else if (w_ascii_streq_caseins(ArchArg, L"sbsa")) {
            g_arch_selection = ARCH_SBSA;
            g_build_sbsa = 1;
        }
        else if (w_ascii_streq_caseins(ArchArg, L"pcbsa")) {
            g_arch_selection = ARCH_PCBSA;
            g_build_pcbsa = 1;
        }
        else if (w_ascii_streq_caseins(ArchArg, L"vbsa")) {
            g_arch_selection = ARCH_VBSA;
        }
    }

    /* Parse level selection flags (-l, -only, -fr). They are mutually exclusive. */
    has_l    = ShellCommandLineGetFlag(ParamPackage, L"-l");
    has_only = ShellCommandLineGetFlag(ParamPackage, L"-only");
    has_fr   = ShellCommandLineGetFlag(ParamPackage, L"-fr");
    if ((has_l ? 1:0) + (has_only?1:0) + (has_fr?1:0) > 1) {
        Print(L"-l, -only, and -fr cannot be combined. Use only one.\n");
        return SHELL_INVALID_PARAMETER;
    }
    if (has_l) {
        CmdLineArg = ShellCommandLineGetValue(ParamPackage, L"-l");
        if (CmdLineArg == NULL) {
            Print(L"Invalid parameter passed for -l\n");
            return SHELL_INVALID_PARAMETER;
        }
        g_level_filter_mode = LVL_FILTER_MAX;
        g_level_value = (UINT32)StrDecimalToUintn(CmdLineArg);
    } else if (has_only) {
        CmdLineArg = ShellCommandLineGetValue(ParamPackage, L"-only");
        if (CmdLineArg == NULL) {
            Print(L"Invalid parameter passed for -only\n");
            return SHELL_INVALID_PARAMETER;
        }
        g_level_filter_mode = LVL_FILTER_ONLY;
        g_level_value = (UINT32)StrDecimalToUintn(CmdLineArg);
    } else if (has_fr) {
        g_level_filter_mode = LVL_FILTER_FR;
        g_level_value = 0;
    } else {
        g_level_filter_mode = LVL_FILTER_NONE; /* will set default later if -a selected */
    }

    /* Parse software view flags (BSA only). They can be combined. */
    has_hyp = ShellCommandLineGetFlag(ParamPackage, L"-hyp");
    has_os  = ShellCommandLineGetFlag(ParamPackage, L"-os");
    has_ps  = ShellCommandLineGetFlag(ParamPackage, L"-ps");
    g_bsa_sw_view_mask = 0;
    if (has_hyp) g_bsa_sw_view_mask |= (1u << SW_HYP);
    if (has_os)  g_bsa_sw_view_mask |= (1u << SW_OS);
    if (has_ps)  g_bsa_sw_view_mask |= (1u << SW_PS);

    /* Parse -skip CLI */
    if (ShellCommandLineGetFlag (ParamPackage, L"-skip")) {
        CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-skip");
        if (CmdLineArg == NULL) {
            Print(L"Invalid parameter passed for -skip\n", 0);
            HelpMsg();
            return SHELL_INVALID_PARAMETER;
        } else {
            g_invalid_arg_seen = FALSE;
            /* Count tokens */
            len = StrLen(CmdLineArg);
            max_tokens = 1; /* k declared at function start */

            for (k = 0; k < len; k++) if (CmdLineArg[k] == L',') max_tokens++;

            if (max_tokens > 0) {
                EStatus = gBS->AllocatePool(EfiBootServicesData,
                                                                    max_tokens * sizeof(RULE_ID_e),
                                                                    (VOID **)&g_skip_rule_list);
                if (EFI_ERROR(EStatus)) {
                    Print(L"Allocate memory for -skip failed\n", 0);
                    return ACS_STATUS_ERR;
                }
            }

            g_skip_rule_count = 0;
            start = 0;
            while (start < len) {
                end = start;
                while (end < len && CmdLineArg[end] != L',')
                    end++;
                /* trim spaces/tabs */
                while (start < end && (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t'))
                    start++;
                while (end > start && (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t'))
                    end--;
                if (end > start) {
                    tlen = end - start;
                    if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
                        tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
                    for (ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
                    wtoken[tlen] = L'\0';
                    map_and_add_rule_token(
                        wtoken,
                        g_skip_rule_list,
                        &g_skip_rule_count,
                        max_tokens
                    );
                }
                start = (end < len) ? end + 1 : end;
            }
            if (g_invalid_arg_seen) {
                return SHELL_INVALID_PARAMETER;
            }
        }
    }

    /* Parse -timeout */
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-timeout");
    if (CmdLineArg == NULL) {
        g_wakeup_timeout = 1;
    } else {
        g_wakeup_timeout = StrDecimalToUintn(CmdLineArg);
        Print(L"Wakeup timeout multiple %d.\n", g_wakeup_timeout);
        if (g_wakeup_timeout > 5)
            g_wakeup_timeout = 5;
    }

    /* Parse verbosity level */
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-v");
    if (CmdLineArg == NULL) {
        g_print_level = G_PRINT_LEVEL;
    } else {
        ReadVerbosity = StrDecimalToUintn(CmdLineArg);
        while (ReadVerbosity/10) {
            g_enable_module |= (1 << ReadVerbosity%10);
            ReadVerbosity /= 10;
        }
        g_print_level = ReadVerbosity;
        if (g_print_level > 5) {
            g_print_level = G_PRINT_LEVEL;
        }
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-mmio")) {
        g_print_mmio = TRUE;
    } else {
        g_print_mmio = FALSE;
    }

    /* -f logfile option */
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-f");
    if (CmdLineArg == NULL) {
        g_acs_log_file_handle = NULL;
    } else {
        Status = ShellOpenFileByName(CmdLineArg, &g_acs_log_file_handle,
                            EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE, 0x0);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to open log file %s\n", CmdLineArg);
            g_acs_log_file_handle = NULL;
        }
    }

    /* get System Last-level cache info */
    CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-slc");
    if (CmdLineArg == NULL) {
        g_sys_last_lvl_cache = 0; /* default value. SLC unknown */
    } else {
        g_sys_last_lvl_cache = StrDecimalToUintn(CmdLineArg);
        if (g_sys_last_lvl_cache > 2 || g_sys_last_lvl_cache < 1) {
            Print(L"Invalid value provided for -slc, Value = %d\n", g_sys_last_lvl_cache);
            g_sys_last_lvl_cache = 0; /* default value. SLC unknown */
        }
    }

    /* If user has pass dtb flag, then dump the dtb in file */
    CmdLineArg  = ShellCommandLineGetValue(ParamPackage, L"-dtb");
    if (CmdLineArg == NULL) {
        g_dtb_log_file_handle = NULL;
    } else {
        Status = ShellOpenFileByName(CmdLineArg, &g_dtb_log_file_handle,
                            EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ | EFI_FILE_MODE_CREATE, 0x0);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to open file for dtb dump %s\n", CmdLineArg);
            g_dtb_log_file_handle = NULL;
        } else {
            val_dump_dtb();
        }
    }

    /* Help message */
    if ((ShellCommandLineGetFlag (ParamPackage, L"-help")) ||
        (ShellCommandLineGetFlag (ParamPackage, L"-h"))) {
        HelpMsg();
        /* returning ACS_STATUS_ERR to block execution of tests */
        return ACS_STATUS_ERR;
    }

    /* no_crypto_ext */
    if ((ShellCommandLineGetFlag (ParamPackage, L"-no_crypto_ext")))
        g_crypto_support = FALSE;


    /* Options with Values: -r <comma-separated rule IDs or rules file> */
    if (ShellCommandLineGetFlag(ParamPackage, L"-r")) {
        CmdLineArg = ShellCommandLineGetValue(ParamPackage, L"-r");
        if (CmdLineArg == NULL) {
                Print(L"Invalid parameter passed for -r\n");
                HelpMsg();
                return SHELL_INVALID_PARAMETER;
        }

        /* If the -r list was split by spaces, collect subsequent args until next option */
        Combined = NULL;
        CombinedLen = 0;
        ShellParams = NULL;

        if (!EFI_ERROR(gBS->OpenProtocol(
                        gImageHandle,
                        &gEfiShellParametersProtocolGuid,
                        (VOID **)&ShellParams,
                        gImageHandle,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL))) {
            /* Walk argv to find the "-r" and concatenate following non-option args */
            for (ai = 0; ai + 1 < ShellParams->Argc; ai++) {
                if (StrCmp(ShellParams->Argv[ai], L"-r") == 0) {
                    /* Start with existing value from GetValue */
                    baseLen = StrLen(CmdLineArg);
                    /* Compute total extra length of trailing segments */
                    extra = 0;
                    for (j = ai + 2; j < ShellParams->Argc; j++) {
                        if (ShellParams->Argv[j][0] == L'-')
                                break;
                        /* +1 for possible inserted comma */
                        extra += StrLen(ShellParams->Argv[j]) + 1;
                    }
                    if (extra > 0) {
                        alloc = baseLen + extra + 1;

                        if (!EFI_ERROR(gBS->AllocatePool(EfiBootServicesData,
                                                        alloc * sizeof(CHAR16),
                                                        (VOID **)&Combined))) {

                            /* Copy base */
                            for (t = 0; t < baseLen; t++) Combined[t] = CmdLineArg[t];
                            CombinedLen = baseLen; Combined[CombinedLen] = L'\0';
                            /* Append segments with delimiter if needed */
                            for (j = ai + 2; j < ShellParams->Argc; j++) {
                                seg = ShellParams->Argv[j];
                                if (seg[0] == L'-') break;
                                /* ensure previous ends with a delimiter */
                                if (CombinedLen > 0) {
                                    last = Combined[CombinedLen - 1];
                                    if (last != L',' && last != L'\n' && last != L'\r') {
                                        Combined[CombinedLen++] = L',';
                                    }
                                }
                                /* append segment */
                                sl = StrLen(seg);
                                for (t = 0; t < sl; t++) Combined[CombinedLen++] = seg[t];
                                Combined[CombinedLen] = L'\0';
                            }
                            CmdLineArg = Combined; /* Use combined list for parsing */
                        }
                    }
                    break;
                }
            }
        }

        /* Try treat the argument as a file path first */
        rfile = NULL;
        if (try_open_readonly(CmdLineArg, &rfile)) {
            /* Read file content */
            raw = NULL;
            raw_len = 0;
            wtext = NULL;
            wlen = 0;

            RS = read_all_bytes(rfile, &raw, &raw_len);
            ShellCloseFile(&rfile);
            if (EFI_ERROR(RS)) {
                Print(L"Failed to read rules file %s\n", CmdLineArg);
                if (Combined) gBS->FreePool(Combined);
                return ACS_STATUS_ERR;
            }

            RS = bytes_to_wchars(raw, raw_len, &wtext, &wlen);
            gBS->FreePool(raw);
            if (EFI_ERROR(RS)) {
                Print(L"Failed to decode rules file %s\n", CmdLineArg);
                if (Combined) gBS->FreePool(Combined);
                return ACS_STATUS_ERR;
            }

            /* Count tokens and allocate list */
            max_tokens = count_rule_tokens_in_text(wtext, wlen);
            if (max_tokens > 0) {
                g_rule_list = (RULE_ID_e *)val_memory_alloc(max_tokens * sizeof(RULE_ID_e));
                if (g_rule_list == NULL) {
                    Print(L"Allocate memory for -r failed\n");
                    gBS->FreePool(wtext);
                    if (Combined) gBS->FreePool(Combined);
                    return ACS_STATUS_ERR;
                }
            }

            g_rule_count = 0;
            g_invalid_arg_seen = FALSE;
            parse_rules_text_into_list(wtext, wlen, g_rule_list, &g_rule_count, max_tokens);
            gBS->FreePool(wtext);
            if (g_invalid_arg_seen) {
                if (Combined) gBS->FreePool(Combined);
                return SHELL_INVALID_PARAMETER;
            }
            } else {
                /* Not a file: parse as single command-line CSV */
                len = StrLen(CmdLineArg);
                max_tokens = 1;
                for (k = 0; k < len; k++) {
                    if (CmdLineArg[k] == L',' ||
                            CmdLineArg[k] == L'\n' ||
                            CmdLineArg[k] == L'\r')
                        max_tokens++;
                }
                if (max_tokens > 0) {
                    g_rule_list = (RULE_ID_e *)val_memory_alloc(max_tokens * sizeof(RULE_ID_e));
                    if (g_rule_list == NULL) {
                        Print(L"Allocate memory for -r failed\n");
                        if (Combined) gBS->FreePool(Combined);
                        return ACS_STATUS_ERR;
                    }
                }
                start = 0;
                g_rule_count = 0;
                g_invalid_arg_seen = FALSE;
                while (start < len) {
                    end = start;
                    while (end < len &&
                                    CmdLineArg[end] != L',' &&
                                    CmdLineArg[end] != L'\n' &&
                                    CmdLineArg[end] != L'\r')
                        end++;
                    /* trim spaces/tabs/CR/LF at boundaries */
                    while (start < end &&
                                    (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t' ||
                                    CmdLineArg[start] == L'\n' || CmdLineArg[start] == L'\r'))
                        start++;
                    while (end > start &&
                                    (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t' ||
                                    CmdLineArg[end-1] == L'\n' || CmdLineArg[end-1] == L'\r'))
                        end--;
                    if (end > start) {
                        tlen = end - start;
                        if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
                            tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
                        for (ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
                        wtoken[tlen] = L'\0';
                        map_and_add_rule_token(wtoken, g_rule_list, &g_rule_count, max_tokens);
                    }
                    start = (end < len) ? end + 1 : end;
                }
                if (g_invalid_arg_seen) {
                    if (Combined) gBS->FreePool(Combined);
                    return SHELL_INVALID_PARAMETER;
                }
            }

        if (Combined) gBS->FreePool(Combined);
    }

    // Options with Values
    if (ShellCommandLineGetFlag (ParamPackage, L"-m")) {
        CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-m");
        if (CmdLineArg == NULL) {
            Print(L"Invalid parameter passed for -m\n", 0);
            HelpMsg();
            return SHELL_INVALID_PARAMETER;
        } else {
            /* Count tokens to size the allocation */
            mlen = StrLen(CmdLineArg);
            max_tokens = 1;
            for (k = 0; k < mlen; k++) {
                if (CmdLineArg[k] == L',' ||
                        CmdLineArg[k] == L'\n' ||
                        CmdLineArg[k] == L'\r')
                    max_tokens++;
            }
            est = gBS->AllocatePool(EfiBootServicesData,
                                                        max_tokens * sizeof(UINT32),
                                                        (VOID **) &g_execute_modules);
            if (EFI_ERROR(est)) {
                Print(L"Allocate memory for -m failed\n", 0);
                return ACS_STATUS_ERR;
            }

            /* Parse CSV of module names */
            g_num_modules = 0;
            start = 0;
            g_invalid_arg_seen = FALSE;
            while (start < mlen) {
                end = start;
                while (end < mlen &&
                                CmdLineArg[end] != L',' &&
                                CmdLineArg[end] != L'\n' &&
                                CmdLineArg[end] != L'\r')
                    end++;
                /* Trim boundaries */
                while (start < end &&
                                (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t' ||
                                CmdLineArg[start] == L'\n' || CmdLineArg[start] == L'\r'))
                    start++;
                while (end > start &&
                                (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t' ||
                                CmdLineArg[end-1] == L'\n' || CmdLineArg[end-1] == L'\r'))
                    end--;
                if (end > start) {
                    tlen = end - start;
                    if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
                        tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
                    for (ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
                    wtoken[tlen] = L'\0';
                    map_and_add_module_token(wtoken, g_execute_modules, &g_num_modules,
                                             max_tokens);
                }
                start = (end < mlen) ? end + 1 : end;
            }
            if (g_invalid_arg_seen) {
                return SHELL_INVALID_PARAMETER;
            }

            if (g_num_modules == 0) {
                Print(L"No valid modules parsed from -m\n", 0);
            }
        }
    }

    /* Options with Values: -skipmodule <comma-separated module names to skip> */
    if (ShellCommandLineGetFlag (ParamPackage, L"-skipmodule")) {
        CmdLineArg  = ShellCommandLineGetValue (ParamPackage, L"-skipmodule");
        if (CmdLineArg == NULL) {
            Print(L"Invalid parameter passed for -skipmodule\n", 0);
            HelpMsg();
            return SHELL_INVALID_PARAMETER;
        } else {
            mlen = StrLen(CmdLineArg);
            max_tokens = 1;
            for (k = 0; k < mlen; k++) {
                if (CmdLineArg[k] == L',' ||
                        CmdLineArg[k] == L'\n' ||
                        CmdLineArg[k] == L'\r')
                    max_tokens++;
            }
            est = gBS->AllocatePool(EfiBootServicesData,
                                                        max_tokens * sizeof(UINT32),
                                                        (VOID **) &g_skip_modules);
            if (EFI_ERROR(est)) {
                Print(L"Allocate memory for -skipmodule failed\n", 0);
                return ACS_STATUS_ERR;
            }

            g_num_skip_modules = 0;
            start = 0;
            g_invalid_arg_seen = FALSE;
            while (start < mlen) {
                end = start;
                while (end < mlen &&
                                CmdLineArg[end] != L',' &&
                                CmdLineArg[end] != L'\n' &&
                                CmdLineArg[end] != L'\r')
                    end++;
                /* Trim boundaries */
                while (start < end &&
                                (CmdLineArg[start] == L' ' || CmdLineArg[start] == L'\t' ||
                                CmdLineArg[start] == L'\n' || CmdLineArg[start] == L'\r'))
                    start++;
                while (end > start &&
                                (CmdLineArg[end-1] == L' ' || CmdLineArg[end-1] == L'\t' ||
                                CmdLineArg[end-1] == L'\n' || CmdLineArg[end-1] == L'\r'))
                    end--;
                if (end > start) {
                    tlen = end - start;
                    if (tlen >= (sizeof(wtoken)/sizeof(wtoken[0])))
                        tlen = (sizeof(wtoken)/sizeof(wtoken[0])) - 1;
                    for (ti = 0; ti < tlen; ti++) wtoken[ti] = CmdLineArg[start + ti];
                    wtoken[tlen] = L'\0';
                    map_and_add_module_token(wtoken, g_skip_modules, &g_num_skip_modules,
                                             max_tokens);
                }
                start = (end < mlen) ? end + 1 : end;
            }
            if (g_invalid_arg_seen) {
                return SHELL_INVALID_PARAMETER;
            }

            if (g_num_skip_modules == 0) {
                Print(L"No valid modules parsed from -skipmodule\n", 0);
            }
        }
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-skip-dp-nic-ms")) {
        g_pcie_skip_dp_nic_ms = TRUE;
    } else {
        g_pcie_skip_dp_nic_ms = FALSE;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-p2p")) {
        g_pcie_p2p = TRUE;
    } else {
        g_pcie_p2p = FALSE;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-cache")) {
        g_pcie_cache_present = TRUE;
    } else {
        g_pcie_cache_present = FALSE;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-el1physkip")) {
        g_el1physkip = TRUE;
    }

    return 0;
}

void
print_selection_summary(void)
{
    UINT32 i;
    RULE_ID_e rid;
    UINT32 mid;
    val_print(ACS_PRINT_TEST, "\nSelected rules: ", 0);
    for (i = 0; i < g_rule_count; i++) {
        rid = g_rule_list[i];
        if (rid < RULE_ID_SENTINEL && rule_id_string[rid] != NULL) {
            val_print(ACS_PRINT_TEST, (char8_t *)rule_id_string[rid], 0);
        } else {
            val_print(ACS_PRINT_TEST, "<INVALID>", 0);
        }
        if (i + 1 < g_rule_count)
            val_print(ACS_PRINT_TEST, ",", 0);
    }
    val_print(ACS_PRINT_TEST, "\n", 0);

    if (g_skip_rule_count > 0 && g_skip_rule_list != NULL) {
        val_print(ACS_PRINT_TEST, "Skipped rules (-skip): ", 0);
        for (i = 0; i < g_skip_rule_count; i++) {
            rid = g_skip_rule_list[i];
            if (rid < RULE_ID_SENTINEL && rule_id_string[rid] != NULL) {
                val_print(ACS_PRINT_TEST, (char8_t *)rule_id_string[rid], 0);
            } else {
                val_print(ACS_PRINT_TEST, "<INVALID>", 0);
            }
            if (i + 1 < g_skip_rule_count)
                val_print(ACS_PRINT_TEST, ",", 0);
        }
        val_print(ACS_PRINT_TEST, "\n", 0);
    }

    if (g_num_modules > 0 && g_execute_modules != NULL) {
        val_print(ACS_PRINT_TEST, "Selected modules (-m): ", 0);
        for (i = 0; i < g_num_modules; i++) {
            mid = g_execute_modules[i];
            if (mid < MODULE_ID_SENTINEL && module_name_string[mid] != NULL) {
                val_print(ACS_PRINT_TEST, (char8_t *)module_name_string[mid], 0);
            } else {
                val_print(ACS_PRINT_TEST, "<INVALID>", 0);
            }
            if (i + 1 < g_num_modules)
                val_print(ACS_PRINT_TEST, ",", 0);
        }
        val_print(ACS_PRINT_TEST, "\n", 0);
    }

    if (g_num_skip_modules > 0 && g_skip_modules != NULL) {
        val_print(ACS_PRINT_TEST, "Skipped modules (-skipmodule): ", 0);
        for (i = 0; i < g_num_skip_modules; i++) {
            mid = g_skip_modules[i];
            if (mid < MODULE_ID_SENTINEL && module_name_string[mid] != NULL) {
                val_print(ACS_PRINT_TEST, (char8_t *)module_name_string[mid], 0);
            } else {
                val_print(ACS_PRINT_TEST, "<INVALID>", 0);
            }
            if (i + 1 < g_num_skip_modules)
                val_print(ACS_PRINT_TEST, ",", 0);
        }
        val_print(ACS_PRINT_TEST, "\n", 0);
    }
}

UINT32
createPeInfoTable (
)
{
    UINT32 Status;
    UINT64 *PeInfoTable;

    PeInfoTable = val_aligned_alloc(SIZE_4K, PE_INFO_TBL_SZ);

    Status = val_pe_create_info_table(PeInfoTable);

    return Status;
}

UINT32
createGicInfoTable (
)
{
    UINT32 Status;
    UINT64 *GicInfoTable;

    GicInfoTable = val_aligned_alloc(SIZE_4K, GIC_INFO_TBL_SZ);

    Status = val_gic_create_info_table(GicInfoTable);

    return Status;
}

VOID
createTimerInfoTable(
)
{
    UINT64 *TimerInfoTable;

    TimerInfoTable = val_aligned_alloc(SIZE_4K, TIMER_INFO_TBL_SZ);

    val_timer_create_info_table(TimerInfoTable);
}

VOID
createWatchdogInfoTable(
)
{
    UINT64 *WdInfoTable;

    WdInfoTable = val_aligned_alloc(SIZE_4K, WD_INFO_TBL_SZ);

    val_wd_create_info_table(WdInfoTable);
}


VOID
createPcieVirtInfoTable(
)
{
    UINT64 *PcieInfoTable;
    UINT64 *IoVirtInfoTable;

    PcieInfoTable   = val_aligned_alloc(SIZE_4K, PCIE_INFO_TBL_SZ);

    val_pcie_create_info_table(PcieInfoTable);

    IoVirtInfoTable = val_aligned_alloc(SIZE_4K, IOVIRT_INFO_TBL_SZ);

    val_iovirt_create_info_table(IoVirtInfoTable);
}

VOID
createPeripheralInfoTable(
)
{
    UINT64 *PeripheralInfoTable;
    UINT64 *MemoryInfoTable;

    PeripheralInfoTable = val_aligned_alloc(SIZE_4K, PERIPHERAL_INFO_TBL_SZ);

    val_peripheral_create_info_table(PeripheralInfoTable);

    MemoryInfoTable = val_aligned_alloc(SIZE_4K, MEM_INFO_TBL_SZ);

    val_memory_create_info_table(MemoryInfoTable);
}

VOID
createSmbiosInfoTable(
)
{
    UINT64 *SmbiosInfoTable;

    SmbiosInfoTable = val_aligned_alloc(SIZE_4K, SMBIOS_INFO_TBL_SZ);

    val_smbios_create_info_table(SmbiosInfoTable);
}

VOID
createPmuInfoTable(
)
{
    UINT64 *PmuInfoTable;

    PmuInfoTable = val_aligned_alloc(SIZE_4K, PMU_INFO_TBL_SZ);

    val_pmu_create_info_table(PmuInfoTable);
}

UINT32
createRasInfoTable(
)
{
    UINT32 status;
    UINT64 *RasInfoTable;

    RasInfoTable = val_aligned_alloc(SIZE_4K, RAS_INFO_TBL_SZ);

    status = val_ras_create_info_table(RasInfoTable);

    return status;
}

VOID
createCacheInfoTable(
)
{
    UINT64 *CacheInfoTable;

    CacheInfoTable = val_aligned_alloc(SIZE_4K, CACHE_INFO_TBL_SZ);

    val_cache_create_info_table(CacheInfoTable);
}

VOID
createMpamInfoTable(
)
{
    UINT64 *MpamInfoTable;

    MpamInfoTable = val_aligned_alloc(SIZE_4K, MPAM_INFO_TBL_SZ);

    val_mpam_create_info_table(MpamInfoTable);
}

VOID
createHmatInfoTable(
)
{
    UINT64 *HmatInfoTable;

    HmatInfoTable = val_aligned_alloc(SIZE_4K, HMAT_INFO_TBL_SZ);

    val_hmat_create_info_table(HmatInfoTable);
}

VOID
createSratInfoTable(
)
{
    UINT64 *SratInfoTable;

    SratInfoTable = val_aligned_alloc(SIZE_4K, SRAT_INFO_TBL_SZ);

    val_srat_create_info_table(SratInfoTable);
}

VOID
createPccInfoTable(
)
{
    UINT64 *PccInfoTable;

    PccInfoTable = val_aligned_alloc(SIZE_4K, PCC_INFO_TBL_SZ);

    val_pcc_create_info_table(PccInfoTable);
}

VOID
createRas2InfoTable(
)
{
    UINT64 *Ras2InfoTable;

    Ras2InfoTable = val_aligned_alloc(SIZE_4K, RAS2_FEAT_INFO_TBL_SZ);

    val_ras2_create_info_table(Ras2InfoTable);
}

VOID
createTpm2InfoTable(
)
{
    UINT64 *Tpm2InfoTable;

    Tpm2InfoTable = val_aligned_alloc(SIZE_4K, TPM2_INFO_TBL_SZ);

    val_tpm2_create_info_table(Tpm2InfoTable);
}

VOID
FlushImage (VOID)
{
    EFI_LOADED_IMAGE_PROTOCOL   *ImageInfo;
    EFI_STATUS Status;
    Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
    if (EFI_ERROR(Status))
    {
        return;
    }

    val_pe_cache_clean_range((UINT64)ImageInfo->ImageBase, (UINT64)ImageInfo->ImageSize);

}
