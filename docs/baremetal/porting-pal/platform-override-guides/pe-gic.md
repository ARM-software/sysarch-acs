# PE & GIC Platform Configuration Guide (MADT-Based, ACS Platform Override Macros)

This note explains how to populate **Processing Element (PE)** and **GIC** platform configuration macros (like those used in SBSA/BSA ACS) for a new ARM-based platform.

It connects the ACPI MADT definitions (GICC, GICD, GICR, ITS, etc.) to ACS-style overrides such as:

```c
/* PE platform config parameters */
#define PLATFORM_OVERRIDE_PE_CNT
#define PLATFORM_OVERRIDE_PE0_INDEX
#define PLATFORM_OVERRIDE_PE0_MPIDR
#define PLATFORM_OVERRIDE_PE0_PMU_GSIV
#define PLATFORM_OVERRIDE_PE0_GMAIN_GSIV
#define PLATFORM_OVERRIDE_PE0_TRBE_INTR
...
/* GIC platform config parameters */
#define PLATFORM_OVERRIDE_GIC_VERSION
#define PLATFORM_OVERRIDE_CORE_COUNT
#define PLATFORM_OVERRIDE_CLUSTER_COUNT
#define PLATFORM_OVERRIDE_GICC_COUNT
#define PLATFORM_OVERRIDE_GICD_COUNT
...
```

Using RD-N2 as an example, we’ll walk through what each thing means and how you should fill it for a new platform.

---

## 1. Conceptual Overview

For ARM systems using the **GIC** interrupt model, ACPI MADT provides:

- One **GICC structure** per logical processor (PE).
- One **GICD structure** describing the Distributor.
- Zero or more **GICR structures** describing redistributor discovery ranges (GICv3+).
- Zero or more **GIC ITS structures** describing ITS units.

Your platform override **PE** config is essentially a distilled view of:

- Which logical CPUs (PEs) exist and their **MPIDR** values.
- For each PE, which **GSIVs** correspond to:
  - PMU interrupts,
  - GIC main maintenance interrupts,
  - Optional TRBE (Trace Buffer Extension) interrupts.

The **GIC** config macros summarize:

- GIC version and topology (core / cluster count).
- Count and base addresses of GICC, GICD, GICR, ITS, and any virtualization components (GICH).

The key idea: **PE + GIC config must be consistent with MADT (and other GIC-related tables) that your platform exposes.**

---

## 2. PE Configuration (PLATFORM_OVERRIDE_PE* Macros)

### 2.1 What a “PE” is here

In ACS tests, a **Processing Element** (PE) means a logical CPU that:

- Has a **GICC entry** in MADT (type 0xB), and
- Has a valid **MPIDR** and is “Enabled” in the GICC Flags.

The ACS PE array is effectively:

```c
struct {
    UINT32 Index;        // logical index 0..N-1
    UINT64 Mpidr;        // MPIDR_EL1 value of this core
    UINT32 PmuGsiv;      // PMU interrupt GSIV used by this core
    UINT32 GMainGsiv;    // GIC maintenance / main GSIV (if relevant)
    UINT32 TrbeGsiv;     // TRBE interrupt GSIV (if supported)
} PlatformPe[];
```

Mapped to macros like:

```c
#define PLATFORM_OVERRIDE_PE0_INDEX        0x0
#define PLATFORM_OVERRIDE_PE0_MPIDR        0x0
#define PLATFORM_OVERRIDE_PE0_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE0_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE0_TRBE_INTR    0x1A
```

### 2.2 High-Level Steps for PE Config

For a new platform:

1. **Enumerate all logical CPUs (PEs) you want ACS to test**
   - Typically all cores present at boot and marked Enabled in MADT GICC structures.

2. **Determine MPIDR for each PE**
   - From hardware view (cluster/core topology).
   - Must match the `MPIDR` field in the GICC structure of that PE.

3. **Determine per-PE interrupt GSIVs**
   - **PMU interrupt GSIV**: the PPI/IRQ used as PMU interrupt for that PE (often same number for all cores, e.g., a PPI).
   - **GMAIN GSIV**: often used for GIC virtual maintenance interrupt or similar; in RD-N2, it is the GIC maintenance PPI.
   - **TRBE interrupt GSIV**: if TRBE is implemented, this is the per-core TRBE PPI value; otherwise 0.

4. **Assign a simple index ordering**
   - `PLATFORM_OVERRIDE_PE<i>_INDEX` is typically equal to `i` (0,1,2,…).
   - Order should match GICC ordering in MADT for sanity (and to match many OS expectations).

5. **Set total PE count**

```c
#define PLATFORM_OVERRIDE_PE_CNT  <number_of_logical_PEs>
```

### 2.3 Mapping MPIDR from Topology

In RD-N2 example:

```c
#define PLATFORM_OVERRIDE_PE0_MPIDR        0x0
#define PLATFORM_OVERRIDE_PE1_MPIDR        0x10000
#define PLATFORM_OVERRIDE_PE2_MPIDR        0x20000
...
#define PLATFORM_OVERRIDE_PE15_MPIDR       0xF0000
```

This pattern corresponds to:

- A cluster/core topology where **Aff0** (bits [7:0]) is the core number and Aff1 ([15:8]) is the cluster ID *or vice versa*.
- MPIDR encoding:
  - On Armv8:
    - Bits [39:32] Aff3
    - Bits [23:16] Aff2
    - Bits [15:8]  Aff1
    - Bits [7:0]   Aff0

Example: `0x0000000000010000` means Aff1=1, Aff0=0 (or depending on your mapping).

For your platform:

1. Decide core numbering (Aff0) and cluster numbering (Aff1/Aff2/Aff3).
2. For each PE, compute the MPIDR value that your hardware uses.
3. Ensure that **this MPIDR matches the value stored in MADT GICC.MPIDR field**.
4. Copy it into `PLATFORM_OVERRIDE_PEx_MPIDR` macros.

### 2.4 PMU, GMAIN, TRBE GSIV values

- **GSIV** (Global System Interrupt Vector) = GIC INTID (for SPIs/PPIs) as seen by the OS.
- These are per-core **PPIs** in most designs:
  - `PMU` PPI: Standard Arm recommended PPI for PMU (e.g., INTID 23 = 0x17).
  - `VGIC maintenance` PPI: typically INTID 25 = 0x19 (example).
  - `TRBE` PPI: specific INTID for Trace Buffer Extension.

You must:

1. Look at your **GIC configuration** (or your firmware/source) to know which PPIs are used for these interrupts.
2. Ensure that **the same GSIV values** are used consistently in:
   - GICC structure fields (Performance Interrupt GSIV, VGIC Maintenance GSIV, SPE/TRBE fields, etc., if used).
   - Your platform override macros.

In RD-N2, all cores share the same GSIV values:

```c
#define PLATFORM_OVERRIDE_PE0_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE0_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE0_TRBE_INTR    0x1A
...
```

For a new platform, if all cores use:

- PMU PPI INTID = 0x23 (35 decimal)
- VGIC maintenance PPI INTID = 0x25 (37 decimal)
- TRBE PPI INTID = 0x26 (38 decimal)

then your macros would be (for each PE):

```c
#define PLATFORM_OVERRIDE_PE0_PMU_GSIV     0x23
#define PLATFORM_OVERRIDE_PE0_GMAIN_GSIV   0x25
#define PLATFORM_OVERRIDE_PE0_TRBE_INTR    0x26
```

If your design uses **per-core unique SPIs** for PMU (less common), you’d fill each PE’s GSIV accordingly.

### 2.5 Worked Example: 4-Core, Single-Cluster Platform

Assume:

- 4 cores, single cluster (Cluster ID=0).
- MPIDR encoding: Aff1 = clusterID, Aff0 = coreID.
  - CPU0: MPIDR = 0x0000_0000_0000_0000
  - CPU1: MPIDR = 0x0000_0000_0000_0001
  - CPU2: MPIDR = 0x0000_0000_0000_0002
  - CPU3: MPIDR = 0x0000_0000_0000_0003
- PMU PPI: 0x17, GMAIN PPI: 0x19, TRBE PPI: 0x1A (same as RD-N2).

You’d define:

```c
#define PLATFORM_OVERRIDE_PE_CNT           4

#define PLATFORM_OVERRIDE_PE0_INDEX        0x0
#define PLATFORM_OVERRIDE_PE0_MPIDR        0x0
#define PLATFORM_OVERRIDE_PE0_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE0_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE0_TRBE_INTR    0x1A

#define PLATFORM_OVERRIDE_PE1_INDEX        0x1
#define PLATFORM_OVERRIDE_PE1_MPIDR        0x1
#define PLATFORM_OVERRIDE_PE1_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE1_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE1_TRBE_INTR    0x1A

#define PLATFORM_OVERRIDE_PE2_INDEX        0x2
#define PLATFORM_OVERRIDE_PE2_MPIDR        0x2
#define PLATFORM_OVERRIDE_PE2_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE2_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE2_TRBE_INTR    0x1A

#define PLATFORM_OVERRIDE_PE3_INDEX        0x3
#define PLATFORM_OVERRIDE_PE3_MPIDR        0x3
#define PLATFORM_OVERRIDE_PE3_PMU_GSIV     0x17
#define PLATFORM_OVERRIDE_PE3_GMAIN_GSIV   0x19
#define PLATFORM_OVERRIDE_PE3_TRBE_INTR    0x1A
```

The corresponding MADT GICC entries must use the same MPIDR values and GSIVs.

---

## 3. GIC Configuration (PLATFORM_OVERRIDE_GIC* Macros)

The GIC macros describe the **global interrupt controller topology** in a compressed way. MADT already provides detailed structures:

- **GICC** (type 0xB) – per-CPU interfaces.
- **GICD** (type 0xC) – distributor.
- **GIC MSI Frame** (type 0xD) – MSI frames.
- **GICR** (type 0xE) – redistributor discovery range.
- **GIC ITS** (type 0xF) – ITS units.

Your RD-N2 config:

```c
#define PLATFORM_OVERRIDE_GIC_VERSION       0x3
#define PLATFORM_OVERRIDE_CORE_COUNT        0x4
#define PLATFORM_OVERRIDE_CLUSTER_COUNT     0x2
#define PLATFORM_OVERRIDE_GICC_COUNT        16
#define PLATFORM_OVERRIDE_GICD_COUNT        0x1
#define PLATFORM_OVERRIDE_GICC_GICRD_COUNT  0x0
#define PLATFORM_OVERRIDE_GICR_GICRD_COUNT  0x1
#define PLATFORM_OVERRIDE_GICITS_COUNT      0x6
#define PLATFORM_OVERRIDE_GICH_COUNT        0x1
#define PLATFORM_OVERRIDE_GICMSIFRAME_COUNT 0x0
#define PLATFORM_OVERRIDE_NONGIC_COUNT      0x0

#define PLATFORM_OVERRIDE_GICC_BASE         0x30000000
#define PLATFORM_OVERRIDE_GICD_BASE         0x30000000
#define PLATFORM_OVERRIDE_GICC_GICRD_BASE   0x0
#define PLATFORM_OVERRIDE_GICR_GICRD_BASE   0x301C0000
#define PLATFORM_OVERRIDE_GICH_BASE         0x2C010000
#define PLATFORM_OVERRIDE_GICITS0_BASE      0x30040000
#define PLATFORM_OVERRIDE_GICITS0_ID        0
#define PLATFORM_OVERRIDE_GICITS1_BASE      0x30080000
#define PLATFORM_OVERRIDE_GICITS1_ID        0x1
#define PLATFORM_OVERRIDE_GICITS2_BASE      0x300C0000
#define PLATFORM_OVERRIDE_GICITS2_ID        0x2
#define PLATFORM_OVERRIDE_GICITS3_BASE      0x30100000
#define PLATFORM_OVERRIDE_GICITS3_ID        0x3
#define PLATFORM_OVERRIDE_GICITS4_BASE      0x30140000
#define PLATFORM_OVERRIDE_GICITS4_ID        0x4
#define PLATFORM_OVERRIDE_GICITS5_BASE      0x30180000
#define PLATFORM_OVERRIDE_GICITS5_ID        0x5
#define PLATFORM_OVERRIDE_GICCIRD_LENGTH    0x0
#define PLATFORM_OVERRIDE_GICRIRD_LENGTH    (0x20000*8)
```

### 3.1 GIC Version and Counts

For a new platform:

1. **Determine GIC version**
   - From hardware / SoC manual or GICD version register:
     - 0x01 – GICv1
     - 0x02 – GICv2
     - 0x03 – GICv3
     - 0x04 – GICv4
   - Set `PLATFORM_OVERRIDE_GIC_VERSION` accordingly.

2. **Core and cluster counts**
   - `PLATFORM_OVERRIDE_CORE_COUNT` – number of cores per cluster (or total cores; check ACS reference, but RD-N2 uses 0x4 with 16 PEs/2 clusters).
   - `PLATFORM_OVERRIDE_CLUSTER_COUNT` – number of CPU clusters.
   - Combined with MPIDRs, ACS uses this to derive topology.

3. **GICC count**
   - `PLATFORM_OVERRIDE_GICC_COUNT` = number of **GICC structures** in MADT (i.e., number of logical CPUs described).
   - Typically equals `PLATFORM_OVERRIDE_PE_CNT` or larger if some PEs are disabled.

4. **GICD count**
   - On most ARM systems, there is **one** GIC distributor → `PLATFORM_OVERRIDE_GICD_COUNT = 0x1`.

5. **GICR and GICRD counts**
   - GICv3+ may use:

     - GICC GICRD base (GICR base per CPU interface in the GICC) or
     - A separate **GICR structure** with a discovery range (as in RD-N2).

   - `PLATFORM_OVERRIDE_GICC_GICRD_COUNT` – number of redistributors described via GICC structures.
   - `PLATFORM_OVERRIDE_GICR_GICRD_COUNT` – number of redistributor discovery ranges described via GICR structures.

6. **ITS, GICH, MSI frames, non-GIC controllers**
   - `PLATFORM_OVERRIDE_GICITS_COUNT` – number of GIC ITS units (MADT type 0xF).
   - `PLATFORM_OVERRIDE_GICH_COUNT` – 1 if virtualization control interface exists and is mapped; 0 otherwise.
   - `PLATFORM_OVERRIDE_GICMSIFRAME_COUNT` – number of GIC MSI frames (MADT type 0xD).
   - `PLATFORM_OVERRIDE_NONGIC_COUNT` – used if there are additional interrupt controllers.

### 3.2 GIC Base Addresses

Fill these from the actual SoC memory map (and ensure consistency with MADT):

- `PLATFORM_OVERRIDE_GICD_BASE` – **GICD Physical Base Address** from MADT GICD structure.
- `PLATFORM_OVERRIDE_GICC_BASE` – **legacy GICC base** (for GICv2 compatibility). On pure GICv3 systems with redistributors, may be 0 or unused.
- `PLATFORM_OVERRIDE_GICC_GICRD_BASE` – base of GICR when described in GICC.
- `PLATFORM_OVERRIDE_GICR_GICRD_BASE` – base of GICR discovery range from GICR structure.
- `PLATFORM_OVERRIDE_GICH_BASE` – GIC virtual interface control block registers.
- `PLATFORM_OVERRIDE_GICITSx_BASE` – base of each GIC ITS unit from ITS structures.
- `PLATFORM_OVERRIDE_GICITSx_ID` – ITS ID from ITS structures.
- `PLATFORM_OVERRIDE_GICRIRD_LENGTH` – length of redistributor discovery range.
- `PLATFORM_OVERRIDE_GICCIRD_LENGTH` – length if redistributors are described via GICC.

All of these must match the addresses and IDs you program in hardware and describe in MADT.

### 3.3 Worked Example: Simple GICv3 Platform

Assume a new platform with:

- 8 cores in 2 clusters (4 cores each).
- GICv3, one Distributor, one Redistributor discovery range, two ITS units.
- Memory map (example):
  - GICD base: `0x2F000000`
  - GICR redistributor discovery base: `0x2F100000`, length: `0x20000 * 8`
  - GICH base: `0x2F200000`
  - ITS0 base: `0x2F400000`, ITS0_ID = 0
  - ITS1 base: `0x2F500000`, ITS1_ID = 1

Then define:

```c
#define PLATFORM_OVERRIDE_GIC_VERSION        0x3
#define PLATFORM_OVERRIDE_CORE_COUNT         0x4    // 4 cores/cluster
#define PLATFORM_OVERRIDE_CLUSTER_COUNT      0x2    // 2 clusters
#define PLATFORM_OVERRIDE_GICC_COUNT         8      // 8 logical CPUs
#define PLATFORM_OVERRIDE_GICD_COUNT         0x1
#define PLATFORM_OVERRIDE_GICC_GICRD_COUNT   0x0
#define PLATFORM_OVERRIDE_GICR_GICRD_COUNT   0x1
#define PLATFORM_OVERRIDE_GICITS_COUNT       0x2
#define PLATFORM_OVERRIDE_GICH_COUNT         0x1
#define PLATFORM_OVERRIDE_GICMSIFRAME_COUNT  0x0
#define PLATFORM_OVERRIDE_NONGIC_COUNT       0x0

#define PLATFORM_OVERRIDE_GICC_BASE          0x0          // if not used
#define PLATFORM_OVERRIDE_GICD_BASE          0x2F000000
#define PLATFORM_OVERRIDE_GICC_GICRD_BASE    0x0
#define PLATFORM_OVERRIDE_GICR_GICRD_BASE    0x2F100000
#define PLATFORM_OVERRIDE_GICH_BASE          0x2F200000

#define PLATFORM_OVERRIDE_GICITS0_BASE       0x2F400000
#define PLATFORM_OVERRIDE_GICITS0_ID         0x0
#define PLATFORM_OVERRIDE_GICITS1_BASE       0x2F500000
#define PLATFORM_OVERRIDE_GICITS1_ID         0x1

#define PLATFORM_OVERRIDE_GICCIRD_LENGTH     0x0
#define PLATFORM_OVERRIDE_GICRIRD_LENGTH     (0x20000 * 8)
```

Your MADT GICD, GICR, ITS entries must use the same base addresses and IDs.

---

## 4. How MADT GICC / GICD / GICR / ITS Tie Back to PE & GIC Macros

- **GICC Structures**
  - Provide:
    - ACPI Processor UID (matches Device(_HID=ACPI0007, _UID=N)).
    - CPU Interface number.
    - Performance interrupt GSIV.
    - GIC virtual maintenance interrupt GSIV.
    - MPIDR.
    - GICR base address (for GICv3, unless GICR discovery range is used).
  - Consistency requirements:
    - `PLATFORM_OVERRIDE_PEx_MPIDR` == `GICC[i].MPIDR` for that PE.
    - `PLATFORM_OVERRIDE_PEx_PMU_GSIV` == `GICC[i].Performance Interrupt GSIV` (if populated).
    - `PLATFORM_OVERRIDE_PEx_GMAIN_GSIV` == `GICC[i].VGIC Maintenance GSIV` (if populated).
    - `PLATFORM_OVERRIDE_PEx_TRBE_INTR` == GICC[i].TRBE interrupt field (if TRBE is present).

- **GICD Structure**
  - Provides the **Distributor base address** and GIC version.
  - Must match `PLATFORM_OVERRIDE_GICD_BASE` and `PLATFORM_OVERRIDE_GIC_VERSION`.

- **GICR Structures**
  - Provide **redistributor discovery base** and length.
  - Must match `PLATFORM_OVERRIDE_GICR_GICRD_BASE` and `PLATFORM_OVERRIDE_GICRIRD_LENGTH`.

- **ITS Structures**
  - Provide base address and ITS ID.
  - Must match `PLATFORM_OVERRIDE_GICITSx_BASE` and `PLATFORM_OVERRIDE_GICITSx_ID` for each ITS.

When these are consistent, ACS PE/GIC tests can discover and exercise the same topology that the OS will see via ACPI.

---

## 5. Implementation Checklist

### PE

- [ ] `PLATFORM_OVERRIDE_PE_CNT` equals number of logical CPUs described by MADT GICC entries with Enabled=1.
- [ ] For each PE `i`:
  - [ ] `PLATFORM_OVERRIDE_PEi_INDEX` is unique (usually i).
  - [ ] `PLATFORM_OVERRIDE_PEi_MPIDR` equals GICC[i].MPIDR.
  - [ ] `PLATFORM_OVERRIDE_PEi_PMU_GSIV` equals Performance Interrupt GSIV from GICC (if populated).
  - [ ] `PLATFORM_OVERRIDE_PEi_GMAIN_GSIV` equals VGIC Maintenance GSIV from GICC (if populated).
  - [ ] `PLATFORM_OVERRIDE_PEi_TRBE_INTR` equals TRBE interrupt GSIV (if TRBE supported), else 0.

### GIC

- [ ] `PLATFORM_OVERRIDE_GIC_VERSION` matches the actual hardware GIC version and MADT GICD “GIC version” field.
- [ ] Core and cluster counts align with MPIDR encoding and the number of GICC entries.
- [ ] `PLATFORM_OVERRIDE_GICC_COUNT` equals number of MADT GICC entries.
- [ ] `PLATFORM_OVERRIDE_GICD_COUNT` is 1 for a single distributor system.
- [ ] `PLATFORM_OVERRIDE_GICC_GICRD_COUNT` and `PLATFORM_OVERRIDE_GICR_GICRD_COUNT` match how redistributors are described (GICC vs GICR).
- [ ] `PLATFORM_OVERRIDE_GICITS_COUNT` equals number of MADT ITS entries.
- [ ] `PLATFORM_OVERRIDE_GICMSIFRAME_COUNT` equals number of MADT MSI Frame entries, if any.
- [ ] `PLATFORM_OVERRIDE_NONGIC_COUNT` is set appropriately if you have additional interrupt controllers.
- [ ] All base addresses (`GICD`, `GICC`, `GICR`, `GICH`, `ITS`) match the physical addresses in the SoC memory map and MADT.
- [ ] Redistributor discovery length is correctly set to cover all redistributors.

Once you follow this mapping, your PE and GIC platform overrides will faithfully reflect the MADT (and hardware), and SBSA/BSA ACS tests for PE and GIC should be able to run on a new platform without surprises.
