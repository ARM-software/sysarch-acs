/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

ENTRY(rel2_entry)

MEMORY {
    /* FVP default: Realm payload base is 0xFDC00000 when ENABLE_RME=1. */
    RAM (rwx) : ORIGIN = 0xFDC00000, LENGTH = 0x00100000 /* 1 MiB */
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
        . = ALIGN(16);
        __stack_start__ = .;
        . = . + 0x7ff8;
        QUAD(0); /* 32 KiB, force stack into binary */
        __stack_end__ = .;
    } > RAM

    /*
     * Dedicated Realm page for alternate-MECID semantic tests. Keep this
     * section loadable so the firmware loader assigns the complete page to
     * the Realm PAS before the payload accesses it.
     */
    .mec_test ORIGIN(RAM) + LENGTH(RAM) - 0x1000 : {
        __mec_test_pa__ = .;
        . = . + 0xff8;
        QUAD(0);
        __mec_test_end__ = .;
    } > RAM

    ASSERT(__stack_end__ <= __mec_test_pa__,
           "Realm payload overlaps the reserved MEC test page")
}
