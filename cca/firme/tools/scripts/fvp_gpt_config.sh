#!/usr/bin/env bash
#
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

if [[ -z "${FVP_GPT_PROFILE+x}" ]]; then
    case "${SUITE:-all}" in
        all|gpt) FVP_GPT_PROFILE=gdi ;;
        *) FVP_GPT_PROFILE=baseline ;;
    esac
fi

case "${FVP_GPT_PROFILE}" in
    baseline)
        FVP_ENABLE_GPC2=0
        FVP_ENABLE_GDI=0
        ;;
    gpc2)
        FVP_ENABLE_GPC2=1
        FVP_ENABLE_GDI=0
        ;;
    gdi)
        FVP_ENABLE_GPC2=1
        FVP_ENABLE_GDI=1
        ;;
    *)
        echo "ERROR: FVP_GPT_PROFILE must be baseline, gpc2 or gdi." >&2
        return 1
        ;;
esac

if [[ "${FVP_GPT_PROFILE}" != "baseline" && \
    ( "${FVP_ENABLE_RME:-1}" != "1" || "${FVP_RME_SUPPORT_LEVEL:-2}" != "2" ) ]]; then
    echo "ERROR: the GPT extension profiles require full FVP RME support." >&2
    return 1
fi

TFA_ENABLE_RME_GDI="${TFA_ENABLE_RME_GDI:-${FVP_ENABLE_GDI}}"
case "${TFA_ENABLE_RME_GDI}:${FVP_ENABLE_GDI}" in
    0:0|2:0|1:1|2:1) ;;
    *)
        echo "ERROR: TFA_ENABLE_RME_GDI must match the model GDI setting (or be 2)." >&2
        return 1
        ;;
esac
