# RAS Platform Override Configuration Guide (ACPI RAS Error Nodes)

This document explains how to populate **RAS-related platform override configuration** for a new Arm-based platform.

---

## 1) What the RAS configuration describes

The RAS configuration describes **error sources** in the system and tells the OS:

- Which components can report architectural or implementation-defined errors
- How error records are accessed:
  - **System Register (SR) interface**, or
  - **MMIO error record groups**
- Which **interrupts** (if any) are used to signal errors
- How error sources are associated with:
  - CPUs
  - memory controllers
  - SMMUs
  - interrupt controllers
  - or vendor-defined devices

Each error source is represented as a **RAS node** with:
- node-specific identification data,
- one interface description,
- zero or more interrupt entries.

---

## 2) How this maps to `platform_override.h` macros

In the platform override header, each RAS node is represented by a **block of macros**.

Typical macro groupings are:

1. **Global sizing and counts**
2. **Per-node header fields**
3. **Per-node resource identification**
4. **Per-node interface description**
5. **Per-node interrupt entries**

Your RD‑N2 example illustrates this flattened representation well.

```c
#define PLATFORM_OVERRIDE_NUM_RAS_NODES       0x1
#define PLATFORM_OVERRIDE_NUM_PE_RAS_NODES    0x1
#define PLATFORM_OVERRIDE_NUM_MC_RAS_NODES    0x0
```

---

## 3) Step-by-step approach for a new platform

### Step 0 — Decide which error sources to expose

Start from hardware capability and firmware design:

- Which blocks implement RAS error registers?
  - CPUs
  - memory controllers
  - SMMUs
  - interrupt controllers
  - vendor-specific logic
- Are error records accessed via:
  - system registers, or
  - memory-mapped register windows?
- Do errors generate interrupts?
  - Fault-handling interrupts
  - Error-recovery interrupts
  - Wired interrupts or MSIs

From this, decide:
- how many **RAS nodes** you need, and
- what **type** each node represents (processor, memory, SMMU, etc.).

---

## 4) Global RAS sizing macros

These macros define overall limits and counts.

```c
#define RAS_MAX_NUM_NODES                     140
#define RAS_MAX_INTR_TYPE                     0x2
#define PLATFORM_OVERRIDE_NUM_RAS_NODES       <total_nodes>
#define PLATFORM_OVERRIDE_NUM_PE_RAS_NODES    <processor_nodes>
#define PLATFORM_OVERRIDE_NUM_MC_RAS_NODES    <memory_nodes>
```

### How to fill these

- `RAS_MAX_NUM_NODES`
  - Upper bound supported by the override implementation
- `PLATFORM_OVERRIDE_NUM_RAS_NODES`
  - Total number of RAS nodes actually populated
- `PLATFORM_OVERRIDE_NUM_PE_RAS_NODES`
  - Number of processor-related RAS nodes
- `PLATFORM_OVERRIDE_NUM_MC_RAS_NODES`
  - Number of memory controller RAS nodes

If you do not expose a certain class (e.g. MC), set its count to zero.

---

## 5) Per-node header fields

Each node has header information that controls layout and interrupt count.

```c
#define PLATFORM_RAS_NODE0_LENGTH             0x0
#define PLATFORM_RAS_NODE0_NUM_INTR_ENTRY     0x0
```

### How to interpret these

- `*_LENGTH`
  - Total size (in bytes) of the node, including:
    - node identification data
    - interface structure
    - interrupt array
- `*_NUM_INTR_ENTRY`
  - Number of interrupt entries associated with this node

These values must be consistent with how the firmware actually lays out the table.

---

## 6) Processor-related RAS nodes

Processor nodes describe error sources associated with CPUs.

```c
#define PLATFORM_RAS_NODE0_PE_PROCESSOR_ID    0x0
#define PLATFORM_RAS_NODE0_PE_RES_TYPE        0x0
#define PLATFORM_RAS_NODE0_PE_FLAGS           0x0
#define PLATFORM_RAS_NODE0_PE_AFF             0x0
#define PLATFORM_RAS_NODE0_PE_RES_DATA        0x0
```

### How to fill these fields

- `PE_PROCESSOR_ID`
  - ACPI `_UID` of the processor this node applies to
  - If the node represents a **global/shared** processor resource, this field must be 0

- `PE_RES_TYPE`
  - Identifies which processor resource is being described:
    - generic processor error source
    - cache
    - TLB
    - other processor-internal structures

- `PE_FLAGS`
  - Indicates whether the node is:
    - per-CPU
    - shared across CPUs
    - global for the system

- `PE_AFF`
  - Processor affinity descriptor
  - Only meaningful for shared resources accessed via system registers
  - Must match the architectural affinity encoding used by the hardware

- `PE_RES_DATA`
  - Resource-specific payload
  - Used to encode cache level/type, TLB level, or generic processor data

**Practical guidance**
- Start with one **generic processor node per CPU**
- Add cache/TLB-specific nodes only if finer-grain reporting is required

---

## 7) Interface description (how error records are accessed)

Each node has exactly one interface description.

```c
#define PLATFORM_RAS_NODE0_INTF_TYPE          0x0
#define PLATFORM_RAS_NODE0_INTF_FLAGS         0x0
#define PLATFORM_RAS_NODE0_INTF_BASE          0x0
#define PLATFORM_RAS_NODE0_INTF_START_REC     0x1
#define PLATFORM_RAS_NODE0_INTF_NUM_REC       0x1
#define PLATFORM_RAS_NODE0_INTF_ERR_REC_IMP   0x0
#define PLATFORM_RAS_NODE0_INTF_ERR_STATUS    0x0
#define PLATFORM_RAS_NODE0_INTF_ADDR_MODE     0x0
```

### Field meanings

- `INTF_TYPE`
  - `0x0` – System Register interface
  - `0x1` – MMIO interface

- `INTF_FLAGS`
  - Shared interface indication
  - Clear-on-read behavior for status registers

- `INTF_BASE`
  - Base address of MMIO error register block
  - Must be zero for system register interfaces

- `INTF_START_REC`
  - Index of first error record handled by this node

- `INTF_NUM_REC`
  - Total number of error records associated with this node

- `INTF_ERR_REC_IMP`
  - Bitmap indicating which records are **not implemented**
  - Bit = 0 → implemented
  - Bit = 1 → not implemented

- `INTF_ERR_STATUS`
  - Bitmap indicating which records support architectural status reporting

- `INTF_ADDR_MODE`
  - Platform-specific selection for record addressing

**Important**
- The meaning of record indices must be consistent with how firmware exposes records
- Off-by-one errors here commonly break OS discovery

---

## 8) Interrupt entries (error signaling)

Each node may define zero or more interrupt entries.

```c
#define PLATFORM_RAS_NODE0_INTR0_TYPE         0x0
#define PLATFORM_RAS_NODE0_INTR0_FLAG         0x1
#define PLATFORM_RAS_NODE0_INTR0_GSIV         0x11
#define PLATFORM_RAS_NODE0_INTR0_ITS_ID       0x0
```

### How to fill interrupt fields

- `INTR_TYPE`
  - `0x0` – fault-handling interrupt
  - `0x1` – error-recovery interrupt

- `INTR_FLAG`
  - `0` – edge-triggered
  - `1` – level-triggered

- `INTR_GSIV`
  - Global System Interrupt value
  - Must be non-zero for wired interrupts
  - Must be zero if MSI is used

- `INTR_ITS_ID`
  - Identifier of the interrupt translation group used for MSI delivery
  - Must be zero for wired interrupts

If hardware uses the same interrupt for both fault handling and recovery, define **two interrupt entries with identical signaling parameters**.

---

## 11) What the RD‑N2 example implies

The RD‑N2 snippet uses placeholder values for many fields. For a real platform you must:

- Compute real node lengths
- Populate meaningful record ranges and bitmaps
- Use real interrupt wiring or MSI identifiers
- Ensure consistency with:
  - CPU topology (MADT)
  - IOMMU topology (IORT)
  - interrupt routing (GIC/ITS)

---
