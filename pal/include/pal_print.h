/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
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

#ifndef __PAL_PRINT_H__
#define __PAL_PRINT_H__

#include "acs_execution_policy.h"

#define ACS_PRINT_ERR   5      /* Only Errors. Use this to trim output to key info */
#define ACS_PRINT_WARN  4      /* Only warnings & errors. Use this to trim output to key info */
#define ACS_PRINT_TEST  3      /* Test description and result descriptions. THIS is DEFAULT */
#define ACS_PRINT_DEBUG 2      /* For Debug statements. contains register dumps etc */
#define ACS_PRINT_INFO  1      /* Print all statements. Do not use unless really needed */

#define PAL_PRINT_IF(verbose, call_expr) \
    do { \
        if ((verbose) >= acs_policy_get_print_level()) { \
            call_expr; \
        } \
    } while (0)

/*
 * PAL_PRINT_FORMAT and PAL_PRINT_LITERAL are backend-specific building blocks.
 * Keep normal PAL call sites on pal_print_msg() with plain source literals.
 * UEFI widens those literals here; baremetal forwards them unchanged.
 */
#if defined(TARGET_BAREMETAL)
void pal_uart_print(int log, const char *fmt, ...);
#if !defined(FAST_PRINT_ENABLE)
#define PAL_PRINT_FORMAT(verbose, string, ...) \
    PAL_PRINT_IF((verbose), pal_uart_print((verbose), (string), ##__VA_ARGS__))
#else
#define PAL_PRINT_FORMAT(verbose, string, ...) \
    PAL_PRINT_IF((verbose), pal_vfastprint((string), ##__VA_ARGS__))
#endif
#define PAL_PRINT_LITERAL(verbose, string, ...) \
    PAL_PRINT_FORMAT((verbose), (string), ##__VA_ARGS__)
#elif defined(TARGET_UEFI)
#include <Library/UefiLib.h>

#define PAL_LOG_MSG_INDENT 7

static inline void pal_print_log_indent(void)
{
    uint32_t i;

    for (i = 0; i < acs_policy_get_log_indent(); i++)
        Print(L"    ");
}

static inline void pal_print_log_prefix(uint32_t verbose)
{
    switch (verbose) {
    case ACS_PRINT_INFO:
    case ACS_PRINT_DEBUG:
        Print(L"   ");
        break;
    case ACS_PRINT_WARN:
        Print(L"   WARN : ");
        break;
    case ACS_PRINT_ERR:
        Print(L"   ERROR: ");
        break;
    case ACS_PRINT_TEST:
    default:
        break;
    }
}

#define PAL_PRINT_FORMAT(verbose, string, ...) \
    PAL_PRINT_IF((verbose), Print((string), ##__VA_ARGS__))

#define PAL_PRINT_LITERAL(verbose, string, ...) \
    do { \
        if ((verbose) >= acs_policy_get_print_level()) { \
            const CHAR16 *_pal_fmt = L"" string; \
            uint32_t _pal_i; \
            bool _pal_new_record = (*_pal_fmt == L'\n'); \
            while (*_pal_fmt == L'\n') { \
                Print(L"\n"); \
                _pal_fmt++; \
            } \
            if (_pal_new_record) { \
                for (_pal_i = 0; _pal_i < PAL_LOG_MSG_INDENT && *_pal_fmt == L' '; _pal_i++) \
                    _pal_fmt++; \
                pal_print_log_indent(); \
                pal_print_log_prefix((verbose)); \
            } \
            Print(_pal_fmt, ##__VA_ARGS__); \
        } \
    } while (0)
#else
#error "pal_print.h requires TARGET_BAREMETAL or TARGET_UEFI"
#endif

/*
 * pal_print_msg() is the standard PAL wrapper used by current call sites.
 * Pass a plain C string literal, not L"...".
 *
 * pal_print_native() is only for rare cases where the caller already has a
 * target-native format string, such as a UEFI CHAR16/L"..." format.
 */
#define pal_print_native(verbose, string, ...) \
    PAL_PRINT_FORMAT((verbose), (string), ##__VA_ARGS__)

#define pal_print_msg(verbose, string, ...) \
    PAL_PRINT_LITERAL((verbose), string, ##__VA_ARGS__)
#endif
