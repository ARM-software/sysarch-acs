# Platform Override Configuration Guide — Timers and Watchdogs

This guide explains how to populate **timer** and **watchdog** platform override macros for a new platform, based on the **Generic Timer Description Table (GTDT)** structures and the **RD-N2 reference macros**.

It focuses on what you need to decide from hardware/firmware, and how those decisions map into the override header fields typically used by SBSA/BSA ACS table generation.

---

## 1) What this configuration describes

The timer configuration published to the OS includes two groups:

1. **Per-processor Generic Timers** (interrupts are PPIs):
   - Secure EL1 timer
   - Non-secure EL1 timer
   - EL2 timer
   - Virtual EL1 timer
   - Virtual EL2 timer (only required if ARMv8.1 VHE is implemented)

2. **Platform (memory-mapped) timers**:
   - **GT Block**: one block can implement up to 8 timer frames (GT0..GT7)
   - **Arm Generic Watchdog**: a standardized watchdog block (refresh frame + control frame)

The OS uses this information during early boot to configure timer interrupts and discover optional platform timers/watchdogs.

---

## 2) Inputs you must collect for a new platform

Before filling overrides, gather:

### Per-processor timer interrupts (PPIs)
For each implemented per-CPU timer, collect:
- **GSIV** (maps 1:1 to the PPI interrupt ID)
- **trigger mode** (edge vs level)
- **polarity** (active-high vs active-low)
- **always-on capability** (can it wake the CPU from low-power states reliably?)

Typically these are platform-wide constants (same GSIV for all CPUs).

### System counter control/read base
From the system memory map:
- `CntControlBase` (CNTCTLBase) physical address *if* exposed via MMIO to non-secure world.
- `CntReadBase` physical address *if* exposed.
If not provided, firmware uses `0xFFFFFFFFFFFFFFFF` in the table, and OS relies on architectural registers.

### Platform timers (GT Blocks)
For each GT Block:
- `CntCtlBase` (GT Block physical base)
- Number of frames implemented
For each frame (GT0..GT7 frame x):
- `CntBaseX` physical address
- `CntEL0BaseX` physical address (or all-ones if not present)
- Physical timer GSIV
- Optional virtual timer GSIV (0 if not implemented)
- Flags (mode/polarity + common flags such as secure/non-secure + always-on)

### Watchdogs
For each watchdog instance:
- Refresh frame base address
- Control frame base address
- GSIV
- Flags (mode/polarity + secure bit)

### Counter frequency
- `CNTFRQ` (system counter frequency), used by bare-metal components or validation harnesses.

---

## 3) Per-processor timer fields (PPI timers)

### What the overrides represent
The following overrides map directly to the per-processor timer GSIV/flags fields:

```c
#define PLATFORM_OVERRIDE_S_EL1_TIMER_GSIV      0x1D
#define PLATFORM_OVERRIDE_NS_EL1_TIMER_GSIV     0x1E
#define PLATFORM_OVERRIDE_NS_EL2_TIMER_GSIV     0x1A
#define PLATFORM_OVERRIDE_VIRTUAL_TIMER_GSIV    0x1B
#define PLATFORM_OVERRIDE_EL2_VIR_TIMER_GSIV    28
```

And flags per timer:

```c
#define PLATFORM_OVERRIDE_S_EL1_TIMER_FLAGS     ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
#define PLATFORM_OVERRIDE_NS_EL1_TIMER_FLAGS    ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
#define PLATFORM_OVERRIDE_NS_EL2_TIMER_FLAGS    ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
#define PLATFORM_OVERRIDE_VIRTUAL_TIMER_FLAGS   ((TIMER_POLARITY << 1) | (TIMER_MODE << 0))
```

### How to fill GSIV values
- These are **GSIVs**, which for ARM timer interrupts correspond to **PPI INTIDs**.
- Use your GIC configuration to confirm the PPI interrupt numbers for:
  - CNTPNSIRQ (Non-secure EL1 physical timer)
  - CNTPHYSIRQ / CNTHPIRQ equivalents (EL2)
  - CNTVIRQ (Virtual EL1)
  - CNTPSIRQ (Secure EL1 timer, if exposed)
  - CNTVIRQ in EL2-VHE context (Virtual EL2 timer)

In many systems, the timer PPIs align with architectural defaults, but **do not assume** — verify from SoC integration.

### How to fill per-processor timer flags
Each timer flags field includes:
- Bit 0: mode (1=edge, 0=level)
- Bit 1: polarity (1=active low, 0=active high)
- Bit 2: always-on capability (for per-processor timers this is in the per-processor flags definition)

Your macro pack uses:
- `TIMER_MODE` → bit 0
- `TIMER_POLARITY` → bit 1

If your platform also encodes “always-on” in these flags, add it per your implementation (some override headers keep always-on separately).

**Guidance**
- Most ARM PPIs are **level-triggered** and **active-high** in typical GIC configurations
- Always-on should be set if:
  - the timer interrupt can wake the CPU from low-power states and
  - the timer context is retained / or re-programmable as required

---

## 4) System counter control base (CNTCTLBase)

The RD-N2 block uses:

```c
#define PLATFORM_OVERRIDE_TIMER_CNTCTL_BASE     0x2a810000
```

This corresponds to the GTDT field:
- `CntControlBase Physical Address`

### How to choose the value
- If the system provides a memory mapped counter control block to non-secure world, set it to that physical address.
- If not provided (common in some designs), the GTDT field is set to `0xFFFFFFFFFFFFFFFF`.

**Important**
- Don’t confuse CNTCTLBase with CNTBase frames of platform timers. CNTCTLBase is the *counter control block*, not a specific GT frame.

---

## 5) Platform timers — GT Block mapping

The RD-N2 example uses a schema that represents a GT Block with multiple frames:

```c
#define PLATFORM_OVERRIDE_PLATFORM_TIMER_COUNT  0x2
#define PLATFORM_OVERRIDE_TIMER_COUNT           0x2
```

Interpretation depends on your table generator, but typically:
- `PLATFORM_OVERRIDE_PLATFORM_TIMER_COUNT` = number of platform timer structures (GT Blocks + Watchdogs)
- `PLATFORM_OVERRIDE_TIMER_COUNT` = number of GT frames/timers populated under a GT Block (or total frame entries)

In RD-N2, they populate **2 GT frames** (frame 0 and frame 1).

### Per-frame fields

Frame 0:
```c
#define PLATFORM_OVERRIDE_TIMER_FRAME_NUM_0     0
#define PLATFORM_OVERRIDE_TIMER_CNTBASE_0       0x2a830000
#define PLATFORM_OVERRIDE_TIMER_CNTEL0BASE_0    0xFFFFFFFFFFFFFFFF
#define PLATFORM_OVERRIDE_TIMER_GSIV_0          0x6d
#define PLATFORM_OVERRIDE_TIMER_VIRT_GSIV_0     0x0
```

Frame 1:
```c
#define PLATFORM_OVERRIDE_TIMER_FRAME_NUM_1     1
#define PLATFORM_OVERRIDE_TIMER_CNTBASE_1       0x2a820000
#define PLATFORM_OVERRIDE_TIMER_CNTEL0BASE_1    0xFFFFFFFFFFFFFFFF
#define PLATFORM_OVERRIDE_TIMER_GSIV_1          0x6c
#define PLATFORM_OVERRIDE_TIMER_VIRT_GSIV_1     0x0
```

These map to GT Block Timer Structure fields:
- Frame number
- CntBaseX
- CntEL0BaseX
- physical timer GSIV
- virtual timer GSIV (0 if not present)

### Flags packing (RD-N2 style)
RD-N2 packs flags into a combined macro:

```c
#define PLATFORM_OVERRIDE_TIMER_PHY_FLAGS_0     0x0
#define PLATFORM_OVERRIDE_TIMER_VIRT_FLAGS_0    0x0
#define PLATFORM_OVERRIDE_TIMER_CMN_FLAGS_0     ((TIMER_IS_ALWAYS_ON_CAPABLE << 1) | (!TIMER_IS_SECURE << 0))
#define PLATFORM_OVERRIDE_TIMER_FLAGS_0         ((PLATFORM_OVERRIDE_TIMER_CMN_FLAGS_0 << 16) |                                                  (PLATFORM_OVERRIDE_TIMER_VIRT_FLAGS_0 << 8) |                                                  (PLATFORM_OVERRIDE_TIMER_PHY_FLAGS_0))
```

Interpretation of this packing commonly is:
- Bits [7:0]   = Physical timer flags (mode/polarity)
- Bits [15:8]  = Virtual timer flags (mode/polarity)
- Bits [31:16] = Common flags (secure + always-on)

#### How to fill the per-frame timer flags
1. **Physical flags** (mode/polarity):
   - Set bit0 = edge/level
   - Set bit1 = active-low/high
2. **Virtual flags** (if implemented):
   - same encoding; if virtual timer not present, keep GSIV = 0 and flags = 0
3. **Common flags**:
   - Secure bit: 1 for secure timers, 0 otherwise
   - Always-on bit: 1 if guaranteed wake/interrupt in low-power states

**Example decision table**
- Non-secure always-on physical timer frame:
  - common: secure=0, always-on=1
- Secure always-on frame:
  - common: secure=1, always-on=1

---

## 6) Counter frequency

```c
#define PLATFORM_BM_TIMER_CNTFRQ         0x5F5E100
```

This is the system counter frequency (CNTFRQ) in Hz.
- Ensure it matches what firmware programs into CNTFRQ_EL0.
- ACS and bare-metal environments use this to validate timer behavior and intervals.

---

## 7) Watchdog configuration (Arm Generic Watchdog)

The watchdog structure provides:
- Refresh frame base
- Control frame base
- GSIV and flags per watchdog timer interrupt

RD-N2 provides:

```c
#define PLATFORM_OVERRIDE_WD_TIMER_COUNT    0x2
#define PLATFORM_OVERRIDE_WD_REFRESH_BASE   0x2A450000
#define PLATFORM_OVERRIDE_WD_CTRL_BASE      0x2A440000
#define PLATFORM_OVERRIDE_WD_GSIV_0         0x6E
#define PLATFORM_OVERRIDE_WD_FLAGS_0        ((!WD_IS_SECURE << 2) | (WD_POLARITY << 1) | (WD_MODE << 0))
#define PLATFORM_OVERRIDE_WD_GSIV_1         0x6F
#define PLATFORM_OVERRIDE_WD_FLAGS_1        ((WD_IS_SECURE << 2) | (WD_POLARITY << 1) | (WD_MODE << 0))
```

### How to fill watchdog base addresses
- `WD_REFRESH_BASE` → RefreshFrame Physical Address
- `WD_CTRL_BASE` → WatchdogControlFrame Physical Address

These are MMIO base addresses of the standard watchdog blocks.

### How to fill watchdog GSIVs
- `WD_GSIV_n` = GSIV (SPI/PPI) used by that watchdog instance
- Many implementations use SPIs for watchdogs; confirm from the GIC interrupt map.

### How to fill watchdog flags
From the watchdog flag definition:
- bit0 = mode (1=edge, 0=level)
- bit1 = polarity (1=active low, 0=active high)
- bit2 = secure timer (1=secure, 0=non-secure)

RD-N2 uses:
- `WD_MODE` → bit0
- `WD_POLARITY` → bit1
- `WD_IS_SECURE` → bit2 (note inversion in one macro; ensure your own logic is consistent)

**Guidance**
- Unless you have a secure-world-only watchdog, most watchdogs used by the OS are **non-secure**.
- Keep secure bit = 0 unless OS is expected to manage it from secure context (rare in standard OS deployments).

---

## 8) Minimal template (fill-in)
Use this to start a new platform override quickly:

```c
/* Per-processor timers */
#define PLATFORM_OVERRIDE_NS_EL1_TIMER_GSIV     <ppi_intid>
#define PLATFORM_OVERRIDE_VIRTUAL_TIMER_GSIV    <ppi_intid>
#define PLATFORM_OVERRIDE_NS_EL2_TIMER_GSIV     <ppi_intid>
#define PLATFORM_OVERRIDE_S_EL1_TIMER_GSIV      <optional_or_0>
#define PLATFORM_OVERRIDE_EL2_VIR_TIMER_GSIV    <vhe_required_or_0>

#define PLATFORM_OVERRIDE_NS_EL1_TIMER_FLAGS    ((<polarity> << 1) | (<mode> << 0) | (<always_on> << 2))
#define PLATFORM_OVERRIDE_VIRTUAL_TIMER_FLAGS   ((<polarity> << 1) | (<mode> << 0) | (<always_on> << 2))
#define PLATFORM_OVERRIDE_NS_EL2_TIMER_FLAGS    ((<polarity> << 1) | (<mode> << 0) | (<always_on> << 2))

/* Counter control base */
#define PLATFORM_OVERRIDE_TIMER_CNTCTL_BASE     <cntctlbase_or_all_ones>

/* Platform GT frames */
#define PLATFORM_OVERRIDE_TIMER_COUNT           <num_frames>

#define PLATFORM_OVERRIDE_TIMER_FRAME_NUM_0     0
#define PLATFORM_OVERRIDE_TIMER_CNTBASE_0       <addr>
#define PLATFORM_OVERRIDE_TIMER_CNTEL0BASE_0    <addr_or_all_ones>
#define PLATFORM_OVERRIDE_TIMER_GSIV_0          <gsiv>
#define PLATFORM_OVERRIDE_TIMER_VIRT_GSIV_0     <gsiv_or_0>
#define PLATFORM_OVERRIDE_TIMER_FLAGS_0         <packed_flags>

/* Watchdog */
#define PLATFORM_OVERRIDE_WD_TIMER_COUNT        <num_wd>
#define PLATFORM_OVERRIDE_WD_REFRESH_BASE       <addr>
#define PLATFORM_OVERRIDE_WD_CTRL_BASE          <addr>
#define PLATFORM_OVERRIDE_WD_GSIV_0             <gsiv>
#define PLATFORM_OVERRIDE_WD_FLAGS_0            ((<secure> << 2) | (<polarity> << 1) | (<mode> << 0))
```
