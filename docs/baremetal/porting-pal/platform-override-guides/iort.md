# IOVIRT / SMMU Platform Configuration Guide (ACPI IORT)

This document explains **how to fill IOVIRT / SMMU platform configuration** for a new Arm-based platform, using the **ACPI IORT (IO Remapping Table)** specification (DEN0049) and the **RD-N2 platform** as a concrete reference.

The intent is to help platform and firmware engineers translate **real hardware topology** (PCIe, SMMUs, StreamIDs, ITS, DMA-capable devices) into the **platform override macros** used by SBSA/BSA ACS.

This guide focuses **only on IOVIRT/IORT**, and is intentionally detailed.

---

## 1. What IORT / IOVIRT Describes

At a high level, IORT tells the OS:

- Which **devices generate transactions** (PCIe RCs, named components)
- Which **SMMU(s)** those transactions pass through
- How **input IDs** (RIDs or implementation-defined IDs) are translated into:
  - **StreamIDs** (for SMMU lookup)
  - **DeviceIDs** (for MSI routing via ITS)
- Which **ITS instance(s)** ultimately receive MSIs

Conceptually, the dataflow is:

```
Requester (PCIe / Named Component)
        |
        |  Input ID (RID or device-specific ID)
        v
Root Complex / Named Component Node
        |
        |  StreamID
        v
SMMUv3 Node
        |
        |  DeviceID
        v
ITS Group Node
        |
        v
GIC ITS (MADT)
```

Every mapping in IORT exists to describe **one step in this pipeline**.

---

## 2. IORT Node Types You Will Typically Use

Most Arm server platforms only need a subset of IORT node types:

| Node Type | Purpose |
|---------|--------|
| **Root Complex (RC)** | Describes PCIe root complexes and RID → StreamID mapping |
| **SMMUv3** | Describes SMMUv3 instances and StreamID → DeviceID mapping |
| **ITS Group** | Groups one or more GIC ITS units |
| **Named Component** | Describes non-PCIe DMA-capable devices |
| **RMR (optional)** | Reserved Memory Regions for DMA devices |

RD-N2 uses:
- 1 Root Complex
- 5 SMMUv3 instances
- 1 ITS Group (containing 5 ITS units)
- 2 Named Components

---

## 3. Platform-Level IOVIRT Macros

These macros define **table-level structure and counts**.

```c
#define IORT_NODE_COUNT               13
#define NUM_ITS_COUNT                 5
#define IOVIRT_ITS_COUNT              1
#define IOVIRT_SMMUV3_COUNT           5
#define IOVIRT_RC_COUNT               1
#define IOVIRT_SMMUV2_COUNT           0
#define IOVIRT_NAMED_COMPONENT_COUNT  2
#define IOVIRT_PMCG_COUNT             0
```

### How to fill these for a new platform

1. **Count every IORT node you will emit**
   - RC nodes
   - SMMU nodes
   - ITS Group nodes
   - Named Components
   - RMR nodes (if any)

2. `NUM_ITS_COUNT`
   - Total number of **GIC ITS units** in hardware
   - Must match **MADT GIC ITS entries**

3. `IOVIRT_ITS_COUNT`
   - Number of **ITS Group nodes** (usually 1)

4. `IOVIRT_SMMUV3_COUNT`
   - Number of SMMUv3 instances

5. `IOVIRT_RC_COUNT`
   - Number of PCIe root complexes

6. `IOVIRT_NAMED_COMPONENT_COUNT`
   - Number of non-PCIe DMA-capable devices.

---

## 4. SMMUv3 Nodes

Each SMMUv3 node represents **one SMMUv3 instance**.

```c
#define IOVIRT_SMMUV3_0_BASE_ADDRESS  0x40000000
#define IOVIRT_SMMUV3_1_BASE_ADDRESS  0x42000000
#define IOVIRT_SMMUV3_2_BASE_ADDRESS  0x44000000
#define IOVIRT_SMMUV3_3_BASE_ADDRESS  0x46000000
#define IOVIRT_SMMUV3_4_BASE_ADDRESS  0x48000000
```

### How to fill for a new platform

For each SMMUv3 instance:

- Use the **MMIO base address** of the SMMU
- Ensure it matches:
  - Hardware address map
  - IORT SMMUv3 node base
  - SMMU base used by firmware/OS

Optional but important fields (not shown in RD-N2 snippet):
- Event IRQ
- PRI IRQ
- GERR IRQ
- Sync IRQ

If your platform supports these, they **must be consistent** across:
- IORT
- Interrupt controller configuration
- Linux `arm-smmu-v3` expectations

---

## 5. Root Complex (RC) Node

The RC node describes **PCIe requesters** and how **RIDs map to StreamIDs**.

```c
#define IOVIRT_RC_PCI_SEG_NUM         0x0
#define IOVIRT_RC_MEMORY_PROPERTIES   0x1
#define IOVIRT_RC_ATS_ATTRIBUTE       0x1
```

### Key RC attributes

- **PCI Segment Number**
  - Typically `0` unless multi-segment PCIe is used

- **Memory Properties**
  - Bitfield describing coherency / shareability
  - `0x1` typically indicates coherent access

- **ATS Attribute**
  - `1` if ATS is supported and enabled
  - Must match PCIe capability exposure

---

## 6. ID Mapping Fundamentals (CRITICAL SECTION)

Almost all IORT bring-up bugs come from **incorrect ID mappings**.

Each mapping describes:

```
Input ID range  --->  Output ID range
         |                 |
     INPUT_BASE        OUTPUT_BASE
     ID_COUNT          OUTPUT_REF
```

### Mapping rule

For an input ID `X`:

```
if (INPUT_BASE <= X <= INPUT_BASE + ID_COUNT)
    OUTPUT_ID = OUTPUT_BASE + (X - INPUT_BASE)
```

⚠️ **ID_COUNT is (number_of_IDs - 1)** — this is the most common mistake.

---

## 7. RC → SMMU (RID → StreamID) Mappings

Example from RD-N2:

```c
#define RC_MAP0_INPUT_BASE            0x0
#define RC_MAP0_ID_COUNT              0x8FFF
#define RC_MAP0_OUTPUT_BASE           0x30000
#define RC_MAP0_OUTPUT_REF            0x5A4
```

Meaning:

- PCIe RIDs `0x0000 – 0x8FFF`
- Map to StreamIDs starting at `0x30000`
- Output goes to **SMMUv3 node reference `0x5A4`**

### How to fill for a new platform

1. Determine **RID ranges**
   - Often grouped by PCIe bus ranges or controllers

2. Decide **StreamID allocation**
   - Fixed window per SMMU is recommended
   - Avoid overlapping StreamID ranges across SMMUs

3. `OUTPUT_REF`
   - Must reference the **correct SMMUv3 node**
   - This is a node offset/reference, not a base address

Repeat mapping entries if:
- Different PCIe buses go to different SMMUs
- You want distinct StreamID windows per bus group

---

## 8. SMMU → ITS (StreamID → DeviceID) Mappings

Each SMMUv3 maps StreamIDs to DeviceIDs consumed by an ITS Group.

Example:

```c
#define SMMUV3_0_ID_MAP1_INPUT_BASE   0x30000
#define SMMUV3_0_ID_MAP1_ID_COUNT     0x8FF
#define SMMUV3_0_ID_MAP1_OUTPUT_BASE  0x30000
#define SMMUV3_0_ID_MAP1_OUTPUT_REF   0x18
```

Meaning:

- StreamIDs `0x30000 – 0x308FF`
- Map to DeviceIDs starting at `0x30000`
- Sent to ITS Group node reference `0x18`

### Best practices

- Keep **DeviceID = StreamID** if possible (simplifies debug)
- Ensure DeviceID range fits within ITS DeviceID width
- All SMMUs that feed the same ITS Group must agree on DeviceID space

---

## 9. Named Component Nodes

Named Components describe **non-PCIe DMA-capable devices**.

Example:

```c
#define IOVIRT_NAMED_0_DEVICE_NAME    "\\_SB_.ETR0"
#define IOVIRT_NAMED_1_DEVICE_NAME    "\\_SB_.DMA0"
```

These nodes still require **ID mappings**:

```c
#define NAMED_COMP0_MAP0_INPUT_BASE   0x0
#define NAMED_COMP0_MAP0_OUTPUT_BASE  0x10000
#define NAMED_COMP0_MAP0_OUTPUT_REF   0xA54
```

### How to fill for a new platform

1. Identify DMA-capable ACPI devices
   - DMA engines
   - Trace units
   - Accelerators

2. Use the **ACPI namespace path**
   - Must match DSDT/SSDT exactly

3. Assign implementation-defined input IDs
   - Often small integers (0,1,2,...)

4. Map to StreamID or DeviceID space
   - Usually directly to an SMMU

---

## 10. ITS Group Nodes

ITS Group nodes bind DeviceIDs to **actual GIC ITS instances**.

Key rules:

- ITS IDs **must match MADT GIC ITS IDs**
- Multiple ITS units may share one ITS Group
- Usually **one ITS Group per SoC**

RD-N2:

```c
#define NUM_ITS_COUNT 5
#define IOVIRT_ITS_COUNT 1
```

Meaning:
- 5 ITS units exist
- 1 ITS Group node aggregates them

---

## 11. Recommended Bring-Up Order

1. Enumerate **GIC ITS units** (MADT)
2. Decide **DeviceID width and layout**
3. Enumerate **SMMUv3 instances**
4. Assign **StreamID windows per SMMU**
5. Populate **RC RID → StreamID mappings**
6. Populate **SMMU StreamID → DeviceID mappings**
7. Add **Named Components**
9. Run ACS IOVIRT / DMA tests

---

## 12. Common Failure Patterns

- Overlapping StreamID windows across SMMUs
- Wrong `ID_COUNT` (off-by-one)
- ITS IDs mismatched between MADT and IORT
- Named Component ACPI paths incorrect
- RC mappings pointing to wrong SMMU node reference

---

This document should be sufficient to derive **IOVIRT/IORT configuration for a new platform** starting from hardware topology and firmware knowledge.
