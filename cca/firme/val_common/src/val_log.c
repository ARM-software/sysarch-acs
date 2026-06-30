/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_log.h"

#include "pal_uart.h"

static pl011_uart_t g_uart;
static uintptr_t g_uart_base;
static uint32_t g_uart_inited;

#ifndef ACS_PRINT_LEVEL
#define ACS_PRINT_LEVEL ACS_PRINT_TEST
#endif

/**
 *   @brief    - Clamp a requested verbosity to a supported range.
 *   @param    - v : Requested verbosity.
 *   @return   - Normalized verbosity.
**/
static print_verbosity_t normalize_verbosity(print_verbosity_t v)
{
    if ((uint32_t)v < (uint32_t)TRACE) {
        return TRACE;
    }
    if ((uint32_t)v > (uint32_t)ERROR) {
        return ERROR;
    }
    return v;
}

static print_verbosity_t g_verbosity = (print_verbosity_t)ACS_PRINT_LEVEL;

__attribute__((weak)) const char *val_world_tag(void)
{
    return "?";
}

/**
 *   @brief    - Decide whether a message should be printed at the current verbosity.
 *   @param    - v : Message verbosity level.
 *   @return   - true if the message should be printed.
**/
static bool should_print(print_verbosity_t v)
{
    return (uint32_t)v >= (uint32_t)g_verbosity;
}

/**
 *   @brief    - Convert a verbosity level to a short string.
 *   @param    - v : Verbosity level.
 *   @return   - String representation.
**/
static const char *indent_str(print_verbosity_t v)
{
    switch (v) {
    case TRACE:
        return "\t\t";
    case TEST:
        return "\t";
    case DEBUG:
    case WARN:
    case ERROR:
    case FATAL:
    default:
        return "\t\t";
    }
}

static const char *msg_prefix(print_verbosity_t v)
{
    switch (v) {
    case WARN:
        return "WARN:";
    case ERROR:
        return "ERR:";
    case FATAL:
        return "FATAL:";
    case DEBUG:
        return "DBG:";
    default:
        return NULL;
    }
}

/**
 *   @brief    - Initialize logging to use the given UART base address.
 *   @param    - uart_base : PL011 UART base address.
 *   @return   - void
**/
void val_log_init(uintptr_t uart_base)
{
    if (g_uart_inited != 0u && g_uart_base == uart_base) {
        return;
    }

    pal_uart_pl011_init(&g_uart, uart_base);
    g_uart_base = uart_base;
    g_uart_inited = 1u;
    g_verbosity = normalize_verbosity(g_verbosity);
}

/**
 *   @brief    - Get the UART base currently used for logging.
 *   @return   - UART base address, or 0 if not initialized yet.
**/
uintptr_t val_log_get_uart_base(void)
{
    return (g_uart_inited != 0u) ? g_uart_base : 0U;
}

/**
 *   @brief    - Set the current runtime verbosity threshold.
 *   @param    - verbosity : New verbosity threshold.
 *   @return   - void
**/
void val_log_set_verbosity(print_verbosity_t verbosity)
{
    g_verbosity = normalize_verbosity(verbosity);
}

/**
 *   @brief    - Get the current runtime verbosity threshold.
 *   @return   - Current verbosity threshold.
**/
print_verbosity_t val_log_get_verbosity(void)
{
    return g_verbosity;
}

/**
 *   @brief    - Output a single character and return the number of characters written.
 *   @param    - c : Character to output.
 *   @return   - Number of characters written (always 1).
**/
static uint32_t putc_count(char c)
{
    if (g_uart_inited == 0u)
        return 0;

    pal_uart_pl011_putc(&g_uart, c);
    return 1;
}

/**
 *   @brief    - Output a string and return the number of characters written.
 *   @param    - s : Null-terminated string to output.
 *   @return   - Number of characters written.
**/
static uint32_t puts_count(const char *s)
{
    uint32_t n = 0;
    if (!s) {
        s = "(null)";
    }
    for (; *s; s++) {
        if (*s == '\n') {
            n += putc_count('\r');
        }
        n += putc_count(*s);
    }
    return n;
}

/**
 *   @brief    - Output a 64-bit value as hex and return the number of characters written.
 *   @param    - v              : Value to output.
 *   @param    - upper          : Use upper-case hex digits if true.
 *   @param    - fixed_width_16 : Print as fixed 16-nybble value if true.
 *   @return   - Number of characters written.
**/
static uint32_t put_u64_hex(uint64_t v, bool upper, bool fixed_width_16)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    bool started = false;
    uint32_t n = 0;

    n += puts_count("0x");
    if (fixed_width_16) {
        for (int i = 15; i >= 0; i--) {
            uint8_t nyb = (uint8_t)((v >> (uint64_t)(i * 4)) & 0xFULL);
            n += putc_count(digits[nyb]);
        }
        return n;
    }

    for (int i = 15; i >= 0; i--) {
        uint8_t nyb = (uint8_t)((v >> (uint64_t)(i * 4)) & 0xFULL);
        if (!started && nyb == 0 && i != 0) {
            continue;
        }
        started = true;
        n += putc_count(digits[nyb]);
    }
    return n;
}

/**
 *   @brief    - Output an unsigned 64-bit value as decimal.
 *   @param    - v : Value to output.
 *   @return   - Number of characters written.
**/
static uint32_t put_u64_dec(uint64_t v)
{
    char buf[32];
    uint32_t n = 0;
    int pos = 0;
    if (v == 0) {
        return putc_count('0');
    }
    while (v && pos < (int)sizeof(buf)) {
        buf[pos++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (pos--) {
        n += putc_count(buf[pos]);
    }
    return n;
}

/**
 *   @brief    - Output a signed 64-bit value as decimal.
 *   @param    - v : Value to output.
 *   @return   - Number of characters written.
**/
static uint32_t put_i64_dec(int64_t v)
{
    uint32_t n = 0;
    if (v < 0) {
        n += putc_count('-');
        /* Avoid overflow for INT64_MIN by converting via unsigned. */
        n += put_u64_dec((uint64_t)(-(v + 1)) + 1);
        return n;
    }
    return put_u64_dec((uint64_t)v);
}

/**
 *   @brief    - Minimal printf core used by the UART logger.
 *   @param    - fmt : Format string.
 *   @param    - ap  : Variadic argument list.
 *   @return   - Number of characters written.
**/
static uint32_t vprintf_core(const char *fmt, va_list ap)
{
    uint32_t n = 0;
    bool is_long = false;
    bool is_long_long = false;
    bool upper = false;
    uint64_t xv = 0;

    for (const char *p = fmt; p && *p; p++) {
        if (*p != '%') {
            n += putc_count(*p);
            continue;
        }

        p++;
        if (*p == '%') {
            n += putc_count('%');
            continue;
        }

        /* Minimal length parsing: l / ll. */
        is_long = false;
        is_long_long = false;
        if (*p == 'l') {
            is_long = true;
            p++;
            if (*p == 'l') {
                is_long_long = true;
                p++;
            }
        }

        switch (*p) {
        case 's':
            n += puts_count(va_arg(ap, const char *));
            break;
        case 'c':
            n += putc_count((char)va_arg(ap, int));
            break;
        case 'd':
        case 'i':
            if (is_long_long) {
                n += put_i64_dec(va_arg(ap, long long));
            } else if (is_long) {
                n += put_i64_dec(va_arg(ap, long));
            } else {
                n += put_i64_dec(va_arg(ap, int));
            }
            break;
        case 'u':
            if (is_long_long) {
                n += put_u64_dec(va_arg(ap, unsigned long long));
            } else if (is_long) {
                n += put_u64_dec(va_arg(ap, unsigned long));
            } else {
                n += put_u64_dec(va_arg(ap, unsigned));
            }
            break;
        case 'p':
            n += put_u64_hex((uint64_t)(uintptr_t)va_arg(ap, void *), false, true);
            break;
        case 'x':
        case 'X':
            upper = (*p == 'X');
            if (is_long_long) {
                xv = (uint64_t)va_arg(ap, unsigned long long);
            } else if (is_long) {
                xv = (uint64_t)va_arg(ap, unsigned long);
            } else {
                xv = (uint64_t)va_arg(ap, unsigned);
            }
            n += put_u64_hex(xv, upper, false);
            break;
        default:
            /* Unknown format: print it verbatim to aid debugging. */
            n += putc_count('%');
            if (is_long_long) {
                n += putc_count('l');
                n += putc_count('l');
            } else if (is_long) {
                n += putc_count('l');
            }
            n += putc_count(*p);
            break;
        }
    }
    return n;
}

/**
 *   @brief    - Print a formatted message if it passes the verbosity filter.
 *   @param    - verbosity : Message verbosity.
 *   @param    - fmt       : Format string.
 *   @param    - ap        : Variadic argument list.
 *   @return   - Number of characters written.
**/
uint32_t val_vprintf(print_verbosity_t verbosity, const char *fmt, va_list ap)
{
    va_list aq;
    uint32_t n = 0;

    if (!should_print(verbosity)) {
        return 0;
    }
    va_copy(aq, ap);
    n = vprintf_core(fmt, aq);
    va_end(aq);
    return n;
}

/**
 *   @brief    - Print a formatted message if it passes the verbosity filter.
 *   @param    - verbosity : Message verbosity.
 *   @param    - fmt       : Format string.
 *   @return   - Number of characters written.
**/
uint32_t val_printf(print_verbosity_t verbosity, const char *fmt, ...)
{
    va_list ap;
    uint32_t n = 0;

    va_start(ap, fmt);
    n = val_vprintf(verbosity, fmt, ap);
    va_end(ap);
    return n;
}

/**
 *   @brief    - Print a tagged formatted message if it passes the verbosity filter.
 *   @param    - verbosity : Message verbosity.
 *   @param    - tag       : Subsystem tag.
 *   @param    - fmt       : Format string.
 *   @param    - ap        : Variadic argument list.
 *   @return   - Number of characters written.
**/
uint32_t val_vlogf(print_verbosity_t verbosity, const char *tag, const char *fmt, va_list ap)
{
    uint32_t n = 0;
    const char *pfx = 0;
    va_list aq;

    if (!should_print(verbosity)) {
        return 0;
    }

    /*
     * Prefix policy:
     * - TRACE/TEST: print the message as-is (no ACS(world), no level prefix).
     * - DEBUG/WARN/ERROR/FATAL: prefix with "ACS(<world>):" + indentation + level prefix.
     */
    if (verbosity != TRACE && verbosity != TEST) {
        if (!tag) {
            tag = val_world_tag();
        }
        n += puts_count("ACS(");
        n += puts_count(tag ? tag : "?");
        n += puts_count("):");
        n += puts_count(indent_str(verbosity));

        pfx = msg_prefix(verbosity);
        if (pfx) {
            n += puts_count(pfx);
            n += putc_count(' ');
        }
    }

    va_copy(aq, ap);
    n += vprintf_core(fmt ? fmt : "", aq);
    va_end(aq);

    n += putc_count('\n');
    return n;
}

/**
 *   @brief    - Print a tagged formatted message if it passes the verbosity filter.
 *   @param    - verbosity : Message verbosity.
 *   @param    - tag       : Subsystem tag.
 *   @param    - fmt       : Format string.
 *   @return   - Number of characters written.
**/
uint32_t val_logf(print_verbosity_t verbosity, const char *tag, const char *fmt, ...)
{
    va_list ap;
    uint32_t n = 0;

    va_start(ap, fmt);
    n = val_vlogf(verbosity, tag, fmt, ap);
    va_end(ap);
    return n;
}
