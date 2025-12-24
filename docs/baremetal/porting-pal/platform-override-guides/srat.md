# SRAT Platform Configuration Guide (Platform Override Macros)

This note explains how to populate SRAT-related platform configuration macros (like those used in SBSA/BSA ACS) for a new Arm-based platform. It uses the RD-N2 example as a reference, but the principles are generic.

---

## 1. High-Level SRAT Design Steps

SRAT describes *system locality* (NUMA / proximity domains) for:

- Processors (GICC Affinity structures on Arm)
- Memory ranges (Memory Affinity structures)
- Optional: GIC ITS, generic initiators (accelerators, coherent devices)

For a new platform, decide the following first:

1. **Number of proximity domains (NUMA nodes)**  
   - UMA system: usually 1 domain (0).  
   - NUMA system: one domain per socket / cluster / memory region, as needed.

2. **CPU → domain mapping**  
   - For each logical CPU (ACPI Processor UID), decide which proximity domain it belongs to.

3. **Memory → domain mapping**  
   - For each contiguous memory range the OS will use, decide which domain it is local to.

4. **Clock domains (optional)**  
   - If the OS need not distinguish clock domains, put all CPUs in clock domain 0.  
   - If there are distinct clock domains (different PLLs/sockets), use different IDs.

The platform override macros simply encode these choices.

---

## 2. Memory Affinity Structures (Type 1)

Each **Memory Affinity** entry associates a memory range with a proximity domain and provides flags like Enabled and HotPluggable.

In ACS-style macros (as in RD-N2):

```c
#define PLATFORM_OVERRIDE_MEM_AFF_CNT       1

#define PLATFORM_SRAT_MEM0_PROX_DOMAIN      0x0
#define PLATFORM_SRAT_MEM0_FLAGS            0x1
#define PLATFORM_SRAT_MEM0_ADDR_BASE        0x8080000000
#define PLATFORM_SRAT_MEM0_ADDR_LEN         0x3F7F7FFFFFF
```

For a new platform:

1. **Enumerate memory regions**
   - Use your platform’s memory map.
   - Include contiguous DRAM regions visible to the OS.
   - Exclude MMIO, reserved firmware, and device regions.

2. **For each region `i` define:**
   - `PLATFORM_SRAT_MEMi_PROX_DOMAIN`  
     Proximity domain ID (e.g., 0, 1, 2, …) this memory is closest to.
   - `PLATFORM_SRAT_MEMi_ADDR_BASE`  
     Base physical address of this region (64-bit).
   - `PLATFORM_SRAT_MEMi_ADDR_LEN`  
     Length in bytes (64-bit).
   - `PLATFORM_SRAT_MEMi_FLAGS`  
     - Bit 0: Enabled  
     - Bit 1: HotPluggable  
     - Bit 2: NonVolatile  
     For normal non-hotplug DRAM → `0x1` (Enabled only).

3. **Count of memory affinity entries:**

```c
#define PLATFORM_OVERRIDE_MEM_AFF_CNT   <number_of_memory_regions>
```

**Sanity checks:**

- No overlapping memory ranges.
- Combined SRAT memory ranges are consistent with the platform memory map.
- Each region’s domain is valid (0 ≤ domain < number of domains).

---

## 3. CPU Affinity via GICC Affinity Structures (Type 3)

On Arm, the **GICC Affinity** structure maps each processor’s **ACPI Processor UID** to a proximity domain and clock domain.

Example from RD-N2:

```c
#define PLATFORM_OVERRIDE_GICC_AFF_CNT      16

#define PLATFORM_SRAT_GICC0_PROX_DOMAIN     0x0
#define PLATFORM_SRAT_GICC0_PROC_UID        0x0
#define PLATFORM_SRAT_GICC0_FLAGS           0x1
#define PLATFORM_SRAT_GICC0_CLK_DOMAIN      0x0

// ... up to GICC15 ...
```

For a new platform:

1. **Determine ACPI Processor UIDs**
   - These come from the MADT GICC structures.
   - Typically one ACPI Processor UID per logical CPU: 0, 1, 2, …

2. **For each logical CPU `n` define:**
   - `PLATFORM_SRAT_GICCn_PROC_UID`  
     Must match the ACPI Processor UID in MADT for that CPU.
   - `PLATFORM_SRAT_GICCn_PROX_DOMAIN`  
     Proximity domain this CPU belongs to.
       - UMA: all CPUs → domain 0.
       - NUMA: group CPUs by socket/cluster and assign domain 0, 1, …
   - `PLATFORM_SRAT_GICCn_FLAGS`  
     - Bit 0: Enabled.  
       Use `0x1` for CPUs present at boot.  
       Use `0x0` for potential future CPUs you want to describe but keep disabled.
   - `PLATFORM_SRAT_GICCn_CLK_DOMAIN`  
     Usually 0 for all CPUs unless you want to distinguish clock domains.

3. **Number of GICC affinity entries:**

```c
#define PLATFORM_OVERRIDE_GICC_AFF_CNT  <number_of_boot_time_cpus_described>
```

OSPM requirements:

- Every CPU started at boot **must** have a corresponding GICC Affinity entry in SRAT (or be associated via `_PXM` if not).
- For hot-added CPUs not described in SRAT, `_PXM` must be used on the processor device (or its ancestor).

---

## 4. Total SRAT Entry Count

In ACS-style overrides, `PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES` is the total count of structures appended after the SRAT header:

```c
NUM_SRAT_ENTRIES =
    MEM_AFF_CNT        // Type 1 memory affinity
  + GICC_AFF_CNT       // Type 3 GICC affinity
  + ITS_AFF_CNT        // Type 4 ITS affinity (optional)
  + GI_AFF_CNT;        // Type 5 generic initiator (optional)
```

Example (RD-N2 excerpt):

- 1 Memory Affinity entry
- 16 GICC Affinity entries
- 0 ITS Affinity entries
- 0 Generic Initiator entries

→ `PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES = 17`

For your platform:

```c
#define PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES      (PLATFORM_OVERRIDE_MEM_AFF_CNT +             PLATFORM_OVERRIDE_GICC_AFF_CNT +            PLATFORM_OVERRIDE_ITS_AFF_CNT  +            PLATFORM_OVERRIDE_GI_AFF_CNT)
```

(If ITS / GI entries are not used, drop those terms.)

---

## 5. Optional: GIC ITS Affinity Structures (Type 4)

If your platform exposes **GIC ITS** entries in MADT, you can describe their proximity via **ITS Affinity** structures. This helps the OS choose nearby memory when allocating ITS tables and command queues.

For each ITS:

- `Proximity Domain` → NUMA node whose memory is closest to the ITS.
- `ITS ID` → must match the `ITS ID` in the MADT GIC ITS entry.

In macro form you might have something like:

```c
#define PLATFORM_OVERRIDE_ITS_AFF_CNT   1

#define PLATFORM_SRAT_ITS0_PROX_DOMAIN  0x0
#define PLATFORM_SRAT_ITS0_ITS_ID       <ITS_ID_from_MADT>
```

If you do not need to advertise ITS locality, you can omit these entries and set `PLATFORM_OVERRIDE_ITS_AFF_CNT` to 0 (or not define it, depending on the ACS codebase).

---

## 6. Optional: Generic Initiator Affinity (Type 5)

Use **Generic Initiator Affinity** structures for devices that initiate transactions and should have a defined locality, for example:

- CXL.mem devices
- Coherent accelerators / GPUs with local HBM
- DMA engines acting as architectural initiators

Key fields:

- `Proximity Domain` → NUMA node whose memory is local to the device.
- `Device Handle Type` → ACPI or PCI.
- `Device Handle` → either:
  - ACPI: `_HID`, `_UID`, or
  - PCI: Segment and BDF (Bus/Device/Function).
- `Flags`:
  - Bit 0: Enabled
  - Bit 1: Architectural transactions (device adheres to the same memory model as host)

Example logical mapping (PCI device):

- PCI device at Segment 0, Bus 0x40, Device 0x1, Function 0.
- Closest to Proximity Domain 2.
- Fully coherent, architectural initiator.

Conceptually:

- Proximity Domain = 2
- Device Handle Type = PCI
- PCI Segment = 0
- PCI BDF = encoded (Bus=0x40, Dev=0x1, Func=0)
- Flags = `0x3` (Enabled + Architectural transactions)

If you do not have such devices, simply omit all Type 5 entries.

---

## 7. Worked Example: Two-NUMA-Node Platform

Assume a new platform with:

- **8 logical CPUs**, ACPI Processor UIDs 0–7.
- **2 NUMA nodes**:
  - Node 0: CPUs 0–3, memory region `[0x0000_0000_8000_0000, 1 GiB)`
  - Node 1: CPUs 4–7, memory region `[0x0000_0001_0000_0000, 1 GiB)`
- Single clock domain (0).
- No ITS or generic initiators described in SRAT.

### Memory Affinity

```c
#define PLATFORM_OVERRIDE_MEM_AFF_CNT   2

/* Memory node 0 */
#define PLATFORM_SRAT_MEM0_PROX_DOMAIN  0x0
#define PLATFORM_SRAT_MEM0_FLAGS        0x1  // Enabled
#define PLATFORM_SRAT_MEM0_ADDR_BASE    0x0000000080000000ULL
#define PLATFORM_SRAT_MEM0_ADDR_LEN     0x0000000040000000ULL

/* Memory node 1 */
#define PLATFORM_SRAT_MEM1_PROX_DOMAIN  0x1
#define PLATFORM_SRAT_MEM1_FLAGS        0x1  // Enabled
#define PLATFORM_SRAT_MEM1_ADDR_BASE    0x0000000100000000ULL
#define PLATFORM_SRAT_MEM1_ADDR_LEN     0x0000000040000000ULL
```

### GICC Affinity

```c
#define PLATFORM_OVERRIDE_GICC_AFF_CNT  8

/* CPUs 0-3 on domain 0 */
#define PLATFORM_SRAT_GICC0_PROX_DOMAIN 0x0
#define PLATFORM_SRAT_GICC0_PROC_UID    0x0
#define PLATFORM_SRAT_GICC0_FLAGS       0x1
#define PLATFORM_SRAT_GICC0_CLK_DOMAIN  0x0

#define PLATFORM_SRAT_GICC1_PROX_DOMAIN 0x0
#define PLATFORM_SRAT_GICC1_PROC_UID    0x1
#define PLATFORM_SRAT_GICC1_FLAGS       0x1
#define PLATFORM_SRAT_GICC1_CLK_DOMAIN  0x0

#define PLATFORM_SRAT_GICC2_PROX_DOMAIN 0x0
#define PLATFORM_SRAT_GICC2_PROC_UID    0x2
#define PLATFORM_SRAT_GICC2_FLAGS       0x1
#define PLATFORM_SRAT_GICC2_CLK_DOMAIN  0x0

#define PLATFORM_SRAT_GICC3_PROX_DOMAIN 0x0
#define PLATFORM_SRAT_GICC3_PROC_UID    0x3
#define PLATFORM_SRAT_GICC3_FLAGS       0x1
#define PLATFORM_SRAT_GICC3_CLK_DOMAIN  0x0

/* CPUs 4-7 on domain 1 */
#define PLATFORM_SRAT_GICC4_PROX_DOMAIN 0x1
#define PLATFORM_SRAT_GICC4_PROC_UID    0x4
#define PLATFORM_SRAT_GICC4_FLAGS       0x1
#define PLATFORM_SRAT_GICC4_CLK_DOMAIN  0x0

#define PLATFORM_SRAT_GICC5_PROX_DOMAIN 0x1
#define PLATFORM_SRAT_GICC5_PROC_UID    0x5
#define PLATFORM_SRAT_GICC5_FLAGS       0x1
#define PLATFORM_SRAT_GICC5_CLK_DOMAIN  0x0

#define PLATFORM_SRAT_GICC6_PROX_DOMAIN 0x1
#define PLATFORM_SRAT_GICC6_PROC_UID    0x6
#define PLATFORM_SRAT_GICC6_FLAGS       0x1
#define PLATFORM_SRAT_GICC6_CLK_DOMAIN  0x0

#define PLATFORM_SRAT_GICC7_PROX_DOMAIN 0x1
#define PLATFORM_SRAT_GICC7_PROC_UID    0x7
#define PLATFORM_SRAT_GICC7_FLAGS       0x1
#define PLATFORM_SRAT_GICC7_CLK_DOMAIN  0x0
```

### Total SRAT Entries

```c
#define PLATFORM_OVERRIDE_NUM_SRAT_ENTRIES      (PLATFORM_OVERRIDE_MEM_AFF_CNT +             PLATFORM_OVERRIDE_GICC_AFF_CNT)
```

This pattern can be adapted directly to your own CPU count, memory map, and NUMA design.

---
