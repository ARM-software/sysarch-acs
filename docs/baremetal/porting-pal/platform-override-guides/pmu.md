# Platform Override Configuration Guide — PMU Nodes (CoreSight-based PMUs)

This document explains how to populate **PMU-related platform override macros** for a new platform, using the **RD-N2 reference values** as a baseline together with the Arm CoreSight-based PMU ACPI specification as the architectural basis.

This guide targets **system or auxiliary PMUs** (memory controller, SMMU, PCIe RC, cache, or other device PMUs) that implement a CoreSight-style register interface and therefore require explicit entries in the platform override data so the ACS can discover them.

> Scope note  
> Architectural CPU PMUs (PMUv3 instances in the processor cores) are discovered directly through architectural registers and do **not** need entries in this override. Only CoreSight-style PMU blocks attached to SoC components are described here.

---

## 1) What this configuration describes

Each PMU node represents:
- one CoreSight-based PMU block,
- associated with a specific **system component** (memory controller, SMMU, PCIe root complex, cache, or ACPI device),
- with MMIO register base(s),
- optional overflow interrupt,
- and a defined **affinity** to a processor or processor container.

The platform override macros are used to construct a table of PMU nodes that an ACS PMU driver can enumerate and bind to component-specific event sets.

---

## 2) High‑level structure of the override

From reference:

```c
#define MAX_NUM_OF_PMU_SUPPORTED              512
#define PLATFORM_OVERRIDE_PMU_NODE_CNT        0x1

#define PLATFORM_PMU_NODE0_BASE0              0x1010028000
#define PLATFORM_PMU_NODE0_BASE1              0x0
#define PLATFORM_PMU_NODE0_TYPE               0x2
#define PLATFORM_PMU_NODE0_PRI_INSTANCE       0x0
#define PLATFORM_PMU_NODE0_SEC_INSTANCE       0x0
#define PLATFORM_PMU_NODE0_DUAL_PAGE_EXT      0x0
```

This breaks down into:

1. **Global limits / counts**
2. **Per‑PMU node properties**
   - base address(es)
   - node type (what component the PMU belongs to)
   - node instance identifiers
   - feature flags (dual‑page support, etc.)

---

## 3) Global PMU limits and counts

### 3.1 `MAX_NUM_OF_PMU_SUPPORTED`

```c
#define MAX_NUM_OF_PMU_SUPPORTED 512
```

This is a **platform or framework upper bound**, not the actual number of PMUs present.

Guidance:
- Set this to a value comfortably larger than the maximum PMUs your platform could ever expose.
- It is often used for static array sizing.

---

### 3.2 `PLATFORM_OVERRIDE_PMU_NODE_CNT`

```c
#define PLATFORM_OVERRIDE_PMU_NODE_CNT 0x1
```

This is the **actual number of PMU nodes** you describe.

For a new platform:
- Count each CoreSight-based PMU block you want the ACS to see.
- Typical systems may have:
  - one PMU per memory controller,
  - one PMU per PCIe root complex,
  - optional PMUs for SMMUs or caches.

---

## 4) Per‑PMU node fields

Each PMU node is described by a group of macros indexed by `NODE<n>`.

---

### 4.1 Base address fields

```c
#define PLATFORM_PMU_NODE0_BASE0 0x1010028000
#define PLATFORM_PMU_NODE0_BASE1 0x0
```

These correspond to the PMU register pages:

- **BASE0**  
  Base address of **Page 0** of the PMU register space.  
  If the PMU is a single‑page implementation, this is the only base you need.

- **BASE1**  
  Base address of **Page 1**, used only if the PMU supports the **dual‑page extension**.

How to fill for a new platform:
- Consult the SoC TRM / integration manual for the PMU block.
- If the PMU implements only one page:
  - set `BASE1 = 0`
  - clear the dual‑page flag.
- If dual‑page is implemented:
  - set `BASE1` to the Page‑1 base address.

---

### 4.2 Dual‑page extension flag

```c
#define PLATFORM_PMU_NODE0_DUAL_PAGE_EXT 0x0
```

This indicates whether the PMU supports the **dual‑page register layout**.

- `0` → single‑page PMU
- `1` → dual‑page PMU (Page 0 + Page 1)

This flag must match the hardware implementation; otherwise register access will be incorrect.

---

### 4.3 Node type

```c
#define PLATFORM_PMU_NODE0_TYPE 0x2
```

The node type defines **which system component** the PMU is associated with.

Common values include:

| Value | Meaning |
|------:|--------|
| `0x00` | Memory controller |
| `0x01` | SMMU |
| `0x02` | PCIe root complex |
| `0x03` | ACPI device |
| `0x04` | CPU cache |

In the RD‑N2 example:
- `0x2` means the PMU is associated with a **PCIe root complex**.

For a new platform:
- Choose the node type based on **what block generates the PMU events**.
- This choice determines how the ACS interprets standard events and masks.

---

### 4.4 Primary and secondary node instance fields

```c
#define PLATFORM_PMU_NODE0_PRI_INSTANCE 0x0
#define PLATFORM_PMU_NODE0_SEC_INSTANCE 0x0
```

These fields **disambiguate which instance** of a component the PMU belongs to.

Their meaning depends on the node type:

#### Examples

- **Memory controller (0x00)**  
  - Primary instance = memory proximity domain (from SRAT)
  - Secondary = 0

- **SMMU (0x01)**  
  - Primary instance = Identifier of the SMMU node in the IORT
  - Secondary = 0

- **PCIe root complex (0x02)**  
  - Primary instance = Identifier of the RC node in the IORT
  - Secondary = 0

- **ACPI device (0x03)**  
  - Primary instance = device _HID
  - Secondary instance = device _UID

- **CPU cache (0x04)**  
  - Primary = 0
  - Secondary = Cache ID from the cache topology configuration

In RD‑N2:
- `PRI_INSTANCE = 0x0` means the PMU is tied to the IORT identifier `0` of the PCIe RC.

---

## 5) Interrupt considerations (conceptual)

Although not shown in RD N 2 override snippet, PMU nodes may support an **overflow interrupt**:

- If present:
  - the interrupt is described by a GSIV
  - flags describe edge/level and wired/MSI
- If absent:
  - GSIV is set to 0

For a new platform:
- Check whether the PMU supports overflow interrupts.
- If it does:
  - use a GSIV that routes to the appropriate interrupt controller,
  - ensure the interrupt is not shared incorrectly.
- If it does not:
  - set GSIV = 0 and rely on polling.

---

## 6) Reading the RD‑N2 example as a template

RD‑N2 defines:
- **one PMU node**
- associated with **PCIe root complex 0**
- single‑page PMU
- no secondary instance

This matches a typical “RC‑level PMU” that counts PCIe traffic or events.

---

## 7) How to extend for a new platform

### 7.1 Multiple PMUs
If your platform has:
- 2 memory controllers
- 2 PCIe root complexes

You might have:
```c
#define PLATFORM_OVERRIDE_PMU_NODE_CNT 0x4
```

And then:
- Node0/1 → memory controllers (type 0x00, instances = proximity domains)
- Node2/3 → PCIe RCs (type 0x02, instances = IORT RC IDs)

---

### 7.2 Cache PMUs
If your platform exposes cache‑level PMUs:
- use node type `0x04`
- secondary instance must match the **cache ID** used in your cache topology configuration.

This ties PMU data directly to a specific cache level or instance.

---

## 8) Minimal template for a new platform

```c
#define MAX_NUM_OF_PMU_SUPPORTED       512
#define PLATFORM_OVERRIDE_PMU_NODE_CNT <N>

/* PMU node 0 */
#define PLATFORM_PMU_NODE0_BASE0        <pmu_base_page0>
#define PLATFORM_PMU_NODE0_BASE1        <pmu_base_page1_or_0>
#define PLATFORM_PMU_NODE0_TYPE         <node_type>
#define PLATFORM_PMU_NODE0_PRI_INSTANCE <primary_instance>
#define PLATFORM_PMU_NODE0_SEC_INSTANCE <secondary_instance>
#define PLATFORM_PMU_NODE0_DUAL_PAGE_EXT <0_or_1>

/* PMU node 1 ... */
```

---
