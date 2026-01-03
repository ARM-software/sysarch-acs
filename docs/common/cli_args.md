# Common CLI Arguments

All ACS binaries and Linux user-space applications (`Bsa.efi`, `Sbsa.efi`,
`PC_bsa.efi`, `Vbsa.efi`, `bsa_app`, `sbsa_app`, `pcbsa_app`, etc.) share the
same runtime interface. Rule identifiers use the prefixes `B_*`, `SB_*`, `P_*`,
or `V_*` depending on the specification. This guide consolidates the option
semantics so individual READMEs can simply reference it.

## CLI reference

| Option | Applies to | Description |
| --- | --- | --- |
| `-a {bsa\|sbsa\|pcbsa}` | xBSA | Choose which checklist the composite binary validates; also gates the level validation for `-l`, `-only`, and `-fr`. |
| `-cache` | BSA & SBSA | Declare that the PCIe hierarchy exposes an address translation cache so PAL enables the related exerciser tests. |
| `-dtb` | BSA | Dump the platform Device Tree Blob to the active filesystem for debug review. |
| `-el1physkip` | VBSA | Skip EL1 register accesses when ACS runs at EL1 (for example, under a hypervisor). Use strictly for debug and document the coverage gap. |
| `-f <path>` | All | Copy UART output to the specified file on the active filesystem (for example, `-f fs0:\logs\run.txt`). |
| `-fr` | All | Include future-requirement (FR) rules for the selected specification. |
| `-help`, `-h` | All | Display the full usage banner inside the UEFI shell. |
| `-l <level>` | All | Execute all rules up to the chosen level (for example, SBSA levels 1-7). |
| `-m <modules>` | All | Run only the listed modules (comma-separated). Valid names include `PE`, `GIC`, `PERIPHERAL`, `MEM_MAP`, `MEMORY`, `PMU`, `RAS`, `SMMU`, `TIMER`, `WATCHDOG`, `NIST`, `PCIE`, `MPAM`, `ETE`, `TPM`, and `POWER_WAKEUP`; unsupported modules in the active binary are ignored. |
| `-mmio` | All | Log every `pal_mmio_read` / `pal_mmio_write` invocation; combine with `-v 1` to focus on MMIO tracing. |
| `-no_crypto_ext` | All | Report that architectural crypto extensions are absent or disabled (for export control or platform reasons). |
| `-only <level>` | All | Run only the rules that match the provided level. |
| `-os`, `-hyp`, `-ps` | BSA | Software-view filters; combine the flags to restrict execution to OS, hypervisor, or platform-security content. |
| `-p2p` | All | Indicate that the PCIe hierarchy supports peer-to-peer transactions so related checks run. |
| `-r <rules\|file>` | All | Run only the supplied rule IDs or the IDs provided in a file (same format as `-skip`). |
| `-skip <rules\|file>` | All | Skip the listed rule IDs (comma-separated) or load IDs from a text file (comments start with `#`; commas/newlines are accepted). |
| `-skip-dp-nic-ms` | All | Skip PCIe exerciser coverage for DisplayPort, network, and mass-storage devices when those endpoints are unavailable. |
| `-skipmodule <modules>` | All | Exclude the listed modules from the run (for example, `-skipmodule PE,GIC`). |
| `-slc <type>` | SBSA | Provide the system last-level cache implementation (`1` for PPTT PE-side cache, `2` for HMAT memory-side cache). |
| `-timeout <1-5>` | All | Scale watchdog and wakeup waits (1 = default/minimum, 5 = maximum delay). |
| `-v <level>` | All | Set verbosity: 5=ERROR, 4=WARN, 3=TEST, 2=DEBUG, 1=INFO. |

Refer to each specification README for other variant-level constraints, rule
prefix conventions, and concrete invocation examples tailored to that ACS
target.

---
*Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.*