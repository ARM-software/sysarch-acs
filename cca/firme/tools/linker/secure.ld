/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

ENTRY(secure_entry)

MEMORY {
    RAM (rwx) : ORIGIN = 0x06000000, LENGTH = 0x00100000 /* 1 MiB */
}

SECTIONS {
    .text : {
        __text_start__ = .;
        *(.text.entry)
        *(.text*)
        *(.rodata*)
        __text_end__ = .;
    } > RAM

    .data : {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > RAM

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end__ = .;
    } > RAM

    /* Keep stack loadable so firmware loaders assign the payload PAS. */
    .stack : {
        . = ALIGN(4096);
        __stack_start__ = .;
        KEEP(*(.mp_stacks))
        __stack_end__ = .;
    } > RAM
}
