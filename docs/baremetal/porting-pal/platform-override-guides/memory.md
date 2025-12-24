# Platform Memory Map Override Guide (Memory Entries)

This guide explains how to populate the **platform memory configuration** macros (often used by validation firmware / ACS harnesses / platform abstraction layers) for a **new platform**, using the **RD‑N2 reference values** as an example.


> This is the **static memory map override** used by the platform layer to describe **physical regions**, optional **identity-mapped virtual aliases**, sizes, and semantic **memory types** (Normal/Device/Reserved/Not‑Populated).

---

## 1) What these entries represent

Each `PLATFORM_OVERRIDE_MEMORY_ENTRY<i>_*` describes **one contiguous address range**:

- **PHY_ADDR**: base physical address of the region
- **VIRT_ADDR**: virtual address at which the region is accessed (often identical to PHY on identity maps)
- **SIZE**: size in bytes
- **TYPE**: how the software should treat the range:
  - `MEMORY_TYPE_NORMAL` – normal cacheable DRAM (usable RAM)
  - `MEMORY_TYPE_DEVICE` – MMIO / Device-nGnRnE / strongly-ordered / non-cacheable region
  - `MEMORY_TYPE_RESERVED` – address range that exists but must **not** be used as general memory (firmware carveout, secure region, MMIO window, etc.)
  - `MEMORY_TYPE_NOT_POPULATED` – address range that *could exist architecturally* but is **not backed by memory** on this platform (holes)

> **Key idea:** the OS/firmware/test-harness uses these entries to know **what is RAM**, **what is MMIO**, and **what must not be touched**.

---

## 2) How to derive the values for a new platform

### Step A — Enumerate *all* address regions you care about
Create a list from:
- SoC memory map / TRM (DRAM windows, MMIO apertures, firmware SRAM, PCIe ECAM/MMIO, GIC, UART, etc.)
- Firmware memory map (carveouts: secure, OP-TEE, SCP, shared memory mailboxes, crashlog, etc.)
- Any “holes” in the DRAM map (e.g., reserved interleaves, highmem gaps)

### Step B — Decide the **entry type** for each region
Use the rule of thumb:

- **Normal DRAM** → `MEMORY_TYPE_NORMAL`
- **Peripheral registers / MMIO apertures** → `MEMORY_TYPE_DEVICE`
- **Firmware carveouts / reserved regions** (even if physically DRAM) → `MEMORY_TYPE_RESERVED`
- **Unimplemented holes** → `MEMORY_TYPE_NOT_POPULATED`

### Step C — Choose PHY vs VIRT mapping
Most platforms use **identity mapping** in early boot / bare-metal tests:

- `VIRT_ADDR == PHY_ADDR`

If your platform uses a fixed offset mapping (e.g., `VA = PA + 0xFFFF_0000_0000_0000`), then:
- `VIRT_ADDR = PHY_ADDR + offset`

### Step D — Ensure entries are **page-aligned** and non-overlapping
Good hygiene (and often required):
- `PHY_ADDR` aligned to at least 4KB (often 64KB/2MB depending on mapping granularity)
- `SIZE` a multiple of the same alignment
- No overlaps between entries
- Prefer fewer, larger ranges unless distinct types require splitting

---

## 3) RD‑N2 example (your reference) annotated

### Provided reference macros
```c
/* Memory config */
#define PLATFORM_OVERRIDE_MEMORY_ENTRY_COUNT        0x4

#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_PHY_ADDR    0x1050000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_VIRT_ADDR   0x1050000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_SIZE        0x4000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_TYPE        MEMORY_TYPE_DEVICE

#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_PHY_ADDR    0xFF600000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_VIRT_ADDR   0xFF600000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_SIZE        0x10000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_TYPE        MEMORY_TYPE_RESERVED

#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_PHY_ADDR    0x80000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_VIRT_ADDR   0x80000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_SIZE        0x60000000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY2_TYPE        MEMORY_TYPE_NORMAL

#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_PHY_ADDR    0xC030000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_VIRT_ADDR   0xC030000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_SIZE        0x20000
#define PLATFORM_OVERRIDE_MEMORY_ENTRY3_TYPE        MEMORY_TYPE_NOT_POPULATED
```

### What each entry implies

| Entry | Range (PA) | Size | Type | Practical meaning |
|------:|------------|------|------|------------------|
| 0 | `0x1050_0000_00` .. `0x1053_FFFF_FF` | 64 MB | Device | A device/MMIO region (e.g., a peripheral window, PCIe MMIO, etc.) |
| 1 | `0xFF60_0000` .. `0xFF60_FFFF` | 64 KB | Reserved | Region exists but must not be treated as normal memory |
| 2 | `0x8000_0000` .. `0xDFFF_FFFF` | 1.5 GB | Normal | DRAM usable by firmware/tests/OS |
| 3 | `0x00C0_3000` .. `0x00C0_4FFF` | 128 KB | Not populated | Hole / unbacked region; touching it should fault/abort |

> Note: The names of the regions (what device or carveout they correspond to) should come from your platform memory map/TRM. The **types** reflect how software must access them.

---

## 4) Template for a new platform

Copy/paste and fill:

```c
/* Memory config */
#define PLATFORM_OVERRIDE_MEMORY_ENTRY_COUNT        <N>

/* Entry0: <RegionName> */
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_PHY_ADDR    <0x...>
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_VIRT_ADDR   <0x...>
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_SIZE        <0x...>
#define PLATFORM_OVERRIDE_MEMORY_ENTRY0_TYPE        <MEMORY_TYPE_NORMAL | MEMORY_TYPE_DEVICE | MEMORY_TYPE_RESERVED | MEMORY_TYPE_NOT_POPULATED>

/* Entry1: <RegionName> */
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_PHY_ADDR    <0x...>
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_VIRT_ADDR   <0x...>
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_SIZE        <0x...>
#define PLATFORM_OVERRIDE_MEMORY_ENTRY1_TYPE        <...>

/* ... */
```

---
