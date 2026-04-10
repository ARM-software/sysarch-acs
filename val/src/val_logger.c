/*
 * Copyright (c) 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_logger.h"

enum { LOG_MAX_STRING_LENGTH = 90 };

static bool collect_log_output;
static char collected_log[LOG_MAX_STRING_LENGTH * 2];
static size_t collected_log_len;

static void val_putc(char c)
{
    if (collect_log_output) {
        if (collected_log_len + 1 < sizeof(collected_log)) {
            collected_log[collected_log_len++] = c;
            collected_log[collected_log_len] = '\0';
        }
        return;
    }
}

/* Keep fields aligned */
/* clang-format off */
struct format_flags {
    uint32_t minus  : 1;
    uint32_t plus   : 1;
    uint32_t space  : 1;
    uint32_t alt    : 1;
    uint32_t zero   : 1;
    uint32_t upper  : 1;
    uint32_t neg    : 1;
};
/* clang-format on */

enum format_base {
    base2 = 2,
    base8 = 8,
    base10 = 10,
    base16 = 16,
};

enum format_length {
    length8 = 8,
    length16 = 16,
    length32 = 32,
    length64 = 64,
};

enum length_mod {
    mod_none = 0,
    mod_h,
    mod_hh,
    mod_l,
    mod_ll,
    mod_bad   /* j, z, t (reject these) */
};

#ifndef STATIC_ASSERT_CHECKS
static_assert(sizeof(char) == sizeof(uint8_t),
          "log expects char to be 8 bits wide");
static_assert(sizeof(short) == sizeof(uint16_t),
          "log expects short to be 16 bits wide");
static_assert(sizeof(int) == sizeof(uint32_t),
          "log expects int to be 32 bits wide");
static_assert(sizeof(long) == sizeof(uint64_t),
          "log expects long to be 64 bits wide");
static_assert(sizeof(long long) == sizeof(uint64_t),
          "log expects long long to be 64 bits wide");
static_assert(sizeof(intmax_t) == sizeof(uint64_t),
          "log expects intmax_t to be 64 bits wide");
static_assert(sizeof(size_t) == sizeof(uint64_t),
          "log expects size_t to be 64 bits wide");
static_assert(sizeof(ptrdiff_t) == sizeof(uint64_t),
          "log expects ptrdiff_t to be 64 bits wide");
#endif

/*
 * These global variables for the log buffer are not static because a test needs
 * to access them directly.
 */
size_t log_buffer_offset;
char log_buffer[LOG_BUFFER_SIZE];

/**
 *   @brief    - Stores a character in a log buffer and outputs it via 'val_putc'
 *   @param    - c  : Input Character
 *   @return   - Sends the character using 'val_putc'
 **/

static void log_putchar(char c)
{
    static char prev;

    log_buffer[log_buffer_offset] = c;
    log_buffer_offset = (log_buffer_offset + 1) % LOG_BUFFER_SIZE;

    /* If we are about to print '\n' and the previous char wasn't '\r',
     * inject '\r' so UART terminals go back to column 0. */
    if (c == '\n' && prev != '\r') {
        char cr = '\r';
        val_putc(cr);
    }

    val_putc(c);
    prev = c;
}

/**
 *   @brief    - Determines length of a string up to a maximum limit
 *   @param    - str    : Input String
 *             - strsz  : Maximum characters to check for string's length
 *   @return   - The length of the null-terminated byte string `str`
 **/

static size_t log_strnlen_s(const char *str, size_t strsz)
{
    if (str == NULL) {
        return 0;
    }

    for (size_t i = 0; i < strsz; ++i) {
        if (str[i] == '\0') {
            return i;
        }
    }

    /* NULL character not found. */
    return strsz;
}

/**
 *   @brief    - Prints a literal string (i.e. '%' is not interpreted specially) to the debug log
 *   @param    - str    : Input literal String
 *   @return   - Number of characters written
 **/

static size_t print_raw_string(const char *str)
{
    const char *c = str;

    for (; *c != '\0'; c++) {
        log_putchar(*c);
    }

    return (size_t)(c - str);
}

/**
 *   @brief    - Prints a formatted string to the debug log
 *   @param    - str        : The full String
 *             - suffix     : Pointer within str that indicates where suffix begins
 *             - min_width  : Minimum width
 *             - flags      : Whether to align to left or right
 *             - fill       : The fill character
 *   @return   - Number of characters written
 **/
static size_t print_string(const char *str, const char *suffix,
               int min_width, struct format_flags *flags,
               char fill)
{
    size_t chars_written = 0;
    size_t prefix_len = (size_t)(suffix - str);
    size_t suffix_len = log_strnlen_s(suffix, LOG_MAX_STRING_LENGTH);
    size_t total_len = prefix_len + suffix_len;

    if (flags->minus) {
        /* Left-aligned: prefix + suffix, then pad with spaces */
        while (str != suffix) {
            chars_written++;
            log_putchar(*str++);
        }

        chars_written += print_raw_string(suffix);

        while (total_len < (size_t)min_width) {
            chars_written++;
            log_putchar(' ');
            total_len++;
        }
        return chars_written;
    }

    /* Right-aligned */
    if (fill == ' ') {
        /* Space padding goes BEFORE prefix/sign */
        while (total_len < (size_t)min_width) {
            chars_written++;
            log_putchar(' ');
            total_len++;
        }

        /* Now print prefix and suffix */
        while (str != suffix) {
            chars_written++;
            log_putchar(*str++);
        }
        chars_written += print_raw_string(suffix);
        return chars_written;
    }

    /* Zero padding (or other fill) goes AFTER prefix, BEFORE digits */
    while (str != suffix) {
        chars_written++;
        log_putchar(*str++);
    }

    while (total_len < (size_t)min_width) {
        chars_written++;
        log_putchar(fill);
        total_len++;
    }

    chars_written += print_raw_string(suffix);
    return chars_written;
}

/**
 *   @brief    - Prints an integer to the debug log
 *   @param    - value      : Integer to be formatted and printed
 *             - base       : Base of the integer
 *             - min_width  : Minimum width of the integer
 *             - flags      : Printf-style flags
 *   @return   - Number of characters written
 **/

static size_t print_int(size_t value, enum format_base base, int min_width,
            struct format_flags *flags)
{
    static const char *digits_lower = "0123456789abcdefxb";
    static const char *digits_upper = "0123456789ABCDEFXB";
    const char *digits = flags->upper ? digits_upper : digits_lower;
    char buf[LOG_MAX_STRING_LENGTH];
    char *ptr = &buf[sizeof(buf) - 1];
    char *num;
    *ptr = '\0';
    do {
        --ptr;
        *ptr = digits[value % base];
        value /= base;
    } while (value);

    /* Num stores where the actual number begins. */
    num = ptr;

    /* Add prefix if requested. */
    if (flags->alt) {
        switch (base) {
        case base16:
            ptr -= 2;
            ptr[0] = '0';
            ptr[1] = digits[16];
            break;

        case base2:
            ptr -= 2;
            ptr[0] = '0';
            ptr[1] = digits[17];
            break;

        case base8:
            ptr--;
            *ptr = '0';
            break;

        case base10:
            /* do nothing */
            break;
        }
    }

    /* Add sign if requested. */
    if (flags->neg) {
        *--ptr = '-';
    } else if (flags->plus) {
        *--ptr = '+';
    } else if (flags->space) {
        *--ptr = ' ';
    }
    return print_string(ptr, num, min_width, flags, flags->zero ? '0' : ' ');
}

/**
 *   @brief    - Parses the optional flags field of a printf-style format
 *   @param    - fmt       : Input format String
 *             - flags     : Store status of formatting flags from input string
 *   @return   - A pointer to the first non-flag character in the string
 **/

static const char *parse_flags(const char *fmt, struct format_flags *flags)
{
    for (;; fmt++) {
        switch (*fmt) {
        case '-':
            flags->minus = true;
            break;

        case '+':
            flags->plus = true;
            break;

        case ' ':
            flags->space = true;
            break;

        case '#':
            flags->alt = true;
            break;

        case '0':
            flags->zero = true;
            break;

        default:
            return fmt;
        }
    }
}

/**
 *   @brief    - Parses the optional length modifier field of a printf-style format
 *   @param    - fmt       : Input String
 *             - length    : Indicates the size of data-type being formatted
 *   @return   - A pointer to the first non-length modifier character in the string
 **/

static const char *parse_length_modifier(const char *fmt,
                     enum format_length *length, enum length_mod *mod)
{
    switch (*fmt) {
    case 'h':
        fmt++;
        if (*fmt == 'h') {
            fmt++;
            *length = length8;
            *mod = mod_hh;
        } else {
            *length = length16;
            *mod = mod_h;
        }
        break;
    case 'l':
        fmt++;
        if (*fmt == 'l') {
            fmt++;
            *length = length64;
            *mod = mod_ll;
        } else {
            *length = length64;
            *mod = mod_l;
        }
        break;

    case 'j':
    case 'z':
    case 't':
        fmt++;
        *length = length64;
        *mod = mod_bad;
        break;

    default:
        *length = length32;
         *mod = mod_none;
        break;
    }

    return fmt;
}

/**
 *   @brief    - Parses the optional minimum width field of a printf-style format
 *   @param    - fmt       : Input String
 *             - args      : List of additional arguments
 *             - flags     : Indicates if the value is negative
 *             - min_width : Integer where parsed minimum width is stored
 *   @return   - A pointer to the first non-digit character in the string
 **/

static const char *parse_min_width(const char *fmt, va_list args,
                   struct format_flags *flags, int *min_width)
{
    int width = 0;

    /* Read minimum width from arguments. */
    if (*fmt == '*') {
        fmt++;
        width = va_arg(args, int);
        if (width < 0) {
            width = -width;
            flags->minus = true;
        }
    } else {
        for (; *fmt >= '0' && *fmt <= '9'; fmt++) {
            width = (width * 10) + (*fmt - '0');
        }
    }

    *min_width = width;

    return fmt;
}

/**
 *   @brief    - Reinterpret an unsigned 64-bit integer as a potentially shorter unsigned
 *               integer according to the length modifier.
 *   @param    - length    : Specifies target-bit width of integer
 *             - value     : Input unsigned integer
 *   @return   - An unsigned integer suitable for passing to `print_int`
 **/

static uint64_t reinterpret_unsigned_int(enum format_length length, uint64_t value)
{
    switch (length) {
    case length8:
        return (uint8_t)value;
    case length16:
        return (uint16_t)value;
    case length32:
        return (uint32_t)value;
    case length64:
        return value;
    }
    return 0;
}

/**
 *   @brief    - Reinterpret an unsigned 64-bit integer as a potentially shorter signed
 *               integer according to the length modifier.
 *   @param    - length    : Specifies width of the integer
 *             - value     : Input unsigned integer value
 *             - flags     : Indicates if the value is negative
 *   @return   - An *unsigned* integer suitable for passing to `print_int`
 **/

static uint64_t reinterpret_signed_int(enum format_length length, uint64_t value,
                struct format_flags *flags)
{
    int64_t signed_value = (int64_t)reinterpret_unsigned_int(length, value);

    switch (length) {
    case length8:
        if ((int8_t)signed_value < 0) {
            flags->neg = true;
            signed_value = (-signed_value) & 0xFF;
        }
        break;
    case length16:
        if ((int16_t)signed_value < 0) {
            flags->neg = true;
            signed_value = (-signed_value) & 0xFFFF;
        }
        break;
    case length32:
        if ((int32_t)signed_value < 0) {
            flags->neg = true;
            signed_value = (-signed_value) & 0xFFFFFFFF;
        }
        break;
    case length64:
        if ((int64_t)signed_value < 0) {
            flags->neg = true;
            if (signed_value == (int64_t)(1ULL << 63)) {
            return (uint64_t)(1ULL << 63);
        }
            signed_value = -signed_value;
        }
        break;
    }

    return (uint64_t)signed_value;
}

/**
 *   @brief    - This function parses and formats a string according to specified format specifiers
 *   @param    - fmt      : Input String
 *             - args     : Arguments are passed as a va_list
 *   @return   - Number of characters written, or `-1` if format string is invalid
 **/

static int val_log(const char *fmt, va_list args)
{
    int chars_written = 0;

    while (*fmt != '\0') {
        switch (*fmt) {
        default:
            chars_written++;
            log_putchar(*fmt);
            fmt++;
            break;

        case '%': {
            struct format_flags flags = {0};
            int min_width = 0;
            enum format_length length = length32;
            enum length_mod mod = mod_none;
            uint64_t value;

            fmt++;
            fmt = parse_flags(fmt, &flags);
            fmt = parse_min_width(fmt, args, &flags, &min_width);
            fmt = parse_length_modifier(fmt, &length, &mod);
            if (mod == mod_bad) {
                chars_written = -1;
                goto out;
             }

            /* Handle the format specifier. */
            switch (*fmt) {
            case '%':
                fmt++;
                chars_written++;
                log_putchar('%');
                break;

            case 'c': {
                char str[2] = {(char)va_arg(args, int), 0};

                fmt++;
                chars_written += print_string(
                    str, str, min_width, &flags, ' ');
                break;
            }

            case 's': {
                char *str = va_arg(args, char *);
                if (str == NULL)
                    str = "(null)";

                fmt++;
                chars_written += print_string(
                    str, str, min_width, &flags, ' ');
                break;
            }

            case 'd':
            case 'i': {
                fmt++;
                if (mod == mod_ll) {
                    value = (uint64_t)(int64_t)va_arg(args, long long);
                } else if (length == length64) {
                    value = (uint64_t)(int64_t)va_arg(args, long);
                } else {
                    value = (uint64_t)(int64_t)va_arg(args, int);
                }
                value = reinterpret_signed_int(length, value,
                                   &flags);

                chars_written += print_int(value, base10,
                               min_width, &flags);
                break;
            }

            case 'b':
                fmt++;
                if (mod == mod_ll) {
                    value = (uint64_t)va_arg(args, unsigned long long);
                 } else if (length == length64) {
                    value = (uint64_t)va_arg(args, unsigned long);
                 } else {
                    value = (uint64_t)va_arg(args, unsigned int);
                 }
                value = reinterpret_unsigned_int(length, value);

                chars_written += print_int(value, base2,
                               min_width, &flags);
                break;

            case 'B':
                fmt++;
                flags.upper = true;
                if (mod == mod_ll) {
                    value = (uint64_t)va_arg(args, unsigned long long);
                } else if (length == length64) {
                    value = (uint64_t)va_arg(args, unsigned long);
                } else {
                   value = (uint64_t)va_arg(args, unsigned int);
                }
                value = reinterpret_unsigned_int(length, value);

                chars_written += print_int(value, base2,
                               min_width, &flags);
                break;

            case 'o':
                fmt++;
                if (mod == mod_ll) {
                    value = (uint64_t)va_arg(args, unsigned long long);
                } else if (length == length64) {
                    value = (uint64_t)va_arg(args, unsigned long);
                } else {
                    value = (uint64_t)va_arg(args, unsigned int);
                }
                value = reinterpret_unsigned_int(length, value);

                chars_written += print_int(value, base8,
                               min_width, &flags);
                break;

            case 'x':
                fmt++;
                if (mod == mod_ll) {
                    value = (uint64_t)va_arg(args, unsigned long long);
                } else if (length == length64) {
                    value = (uint64_t)va_arg(args, unsigned long);
                } else{
                  value = (uint64_t)va_arg(args, unsigned int);
                }
                value = reinterpret_unsigned_int(length, value);

                chars_written += print_int(value, base16,
                               min_width, &flags);
                break;

            case 'X':
                fmt++;
                flags.upper = true;
                if (mod == mod_ll) {
                    value = (uint64_t)va_arg(args, unsigned long long);
                } else if (length == length64) {
                    value = (uint64_t)va_arg(args, unsigned long);
                } else {
                   value = (uint64_t)va_arg(args, unsigned int);
                }
                value = reinterpret_unsigned_int(length, value);

                chars_written += print_int(value, base16,
                               min_width, &flags);
                break;

            case 'u':
                fmt++;
                if (mod == mod_ll) {
                    value = (uint64_t)va_arg(args, unsigned long long);
                } else if (length == length64) {
                    value = (uint64_t)va_arg(args, unsigned long);
                } else {
                   value = (uint64_t)va_arg(args, unsigned int);
                }
                value = reinterpret_unsigned_int(length, value);

                chars_written += print_int(value, base10,
                               min_width, &flags);
                break;

            case 'p':
                fmt++;
                value = (uint64_t)(uintptr_t)va_arg(args, void *);
                min_width = sizeof(size_t) * 2 + 2;
                flags.zero = true;
                flags.alt = true;

                chars_written += print_int(value, base16,
                               min_width, &flags);
                break;

            default:
                chars_written = -1;
                goto out;
            }
        }
        }
    }

out:
    return chars_written;
}

/**
 *   @brief    - This function prints the given string and data onto the uart
 *   @param    - verbosity  : Print Verbosity level
 *             - msg        : Input String
 *             - ...        : ellipses for variadic args
 *   @return   - SUCCESS((Any positive number for character written)/FAILURE(0)
 **/

uint32_t val_printf(print_verbosity_t verbosity, const char *msg, ...)
{
    int chars_written = 0;
    static bool lastWasNewline = true;
    static bool prefixPrinted;
    char formatted_msg[LOG_MAX_STRING_LENGTH];
    va_list args;

    if (msg == NULL)
        return 0;

    collect_log_output = true;
    collected_log_len = 0;
    collected_log[0] = '\0';

    /* New line => allow prefix again */
    if (lastWasNewline)
        prefixPrinted = false;

    /* Emit any leading blank lines cleanly (and don't prefix blank lines) */
    while (*msg == '\n') {
        print_raw_string("\r\n");
        lastWasNewline = true;
        prefixPrinted  = false;
        msg++;
    }

    /* If msg was only newlines */
    if (*msg == '\0') {
        collect_log_output = false;
        if (collected_log_len > 0)
            pal_print((uint64_t)(uintptr_t)collected_log);
        return 0;
    }

    /* Print prefix exactly once per logical line (supports multi-call line assembly) */
    if (!prefixPrinted) {
        switch (verbosity)
        {
            case TRACE:
                 print_raw_string("\t");
                 break;
            case DEBUG:
                 print_raw_string("\t");
                 break;
            case INFO:
                 print_raw_string("");
                 break;
            case WARN:
                 print_raw_string("\tWARN : ");
                 break;
            case ERROR:
                 print_raw_string("\tERROR: ");
                 break;
            case FATAL:
                 print_raw_string("\tFATAL: ");
                 break;
            default:
                 break;
        }
        prefixPrinted = true;
    }

    /* Bounded scan: we only safely inspect up to N-2 chars */
    const size_t max_scan = LOG_MAX_STRING_LENGTH - 2;
    size_t len = log_strnlen_s(msg, max_scan);

    va_start(args, msg);

    /*
     * 3 cases:
     *  A) msg ends with '\n' convert final LF to CRLF
     *  B) msg is likely longer than buffer size => truncate to N-3 and force CRLF
     *  C) short msg without '\n'
     */

    /* Case A: ends with '\n'*/
    if (len > 0 && msg[len - 1] == '\n')
    {
        val_mem_copy(formatted_msg, msg, len - 1);
        formatted_msg[len - 1] = '\r';
        formatted_msg[len]     = '\n';
        formatted_msg[len + 1] = '\0';

        chars_written = val_log(formatted_msg, args);
        lastWasNewline = true;
        prefixPrinted  = false;
    }
    /* Case B: likely truncated (no '\0' found within max_scan) */
    else if (len == max_scan)
    {
        const char trunc_msg[] = "<truncated>\r\n";
        const size_t trunc_len = sizeof(trunc_msg) - 1;

        size_t perm_len = LOG_MAX_STRING_LENGTH - trunc_len - 1;

        if (perm_len > len)
            perm_len = len;

        val_mem_copy(formatted_msg, msg, perm_len);
        formatted_msg[perm_len] = '\0';

        if (perm_len > 0)
            chars_written += (int)print_raw_string(formatted_msg);

        chars_written += (int)print_raw_string(trunc_msg);

        lastWasNewline = true;
        prefixPrinted  = false;  /* next line should get prefix */
     }

    /* Case C: short, no trailing '\n' */
    else
    {
        chars_written = val_log(msg, args);
        lastWasNewline = false;
    }

    va_end(args);

    collect_log_output = false;

    if (chars_written < 0)
    return 0;

    if (collected_log_len > 0)
        pal_print((uint64_t)(uintptr_t)collected_log);

    return (uint32_t)chars_written;
}

/**
  @brief  Copy memory from source to destination

  @param  dst  Destination buffer
  @param  src  Source buffer
  @param  len  Number of bytes to copy

  @return Pointer to destination buffer
**/
void val_mem_copy(char *dest, const char *src, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        dest[i] = src[i];
}
