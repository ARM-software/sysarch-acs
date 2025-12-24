# MPAM and PCC Platform Configuration Guide (Platform Override Macros)

This note explains how to populate MPAM- and PCC-related platform configuration macros (like those used in SBSA/BSA ACS) for a new Arm-based platform.

The focus is on how to *think about* your platform and then translate that into ACS-style macros such as:

```c
/* MPAM Config */
#define MPAM_MAX_MSC_NODE
#define MPAM_MAX_RSRC_NODE
#define PLATFORM_MPAM_MSC_COUNT
...
/* PCC Config */
#define PLATFORM_PCC_SUBSPACE_COUNT
#define PLATFORM_PCC_SUBSPACE0_...
```

---

## 1. Overview: What MPAM and PCC Describe

### 1.1 MPAM

MPAM (Memory System Resource Partitioning And Monitoring) is described to the OS using the **ACPI MPAM table**. Conceptually:

- **MSCs (Memory System Components)** are logical blocks that expose MPAM registers (partitioning and monitoring controls).
- Each **MSC node** in the MPAM table describes:
  - How software accesses the MSC (MMIO or PCC).
  - Interrupts for overflow and error reporting (optional).
  - MAX_NRDY timing for configuration changes.
  - The set of **resources** managed by that MSC (cache storage, memory bandwidth, interconnect link, etc).
- Each **Resource node** describes:
  - Which logical resource is controlled (cache instance, memory domain, SMMU TLB, interconnect, etc).
  - Whether RIS (Resource Instance Selection) is used.
  - The **locator** describing *where* the resource is in the system (PPTT cache node, SRAT proximity domain, IORT node, etc).

The job of the platform configuration is therefore:

1. Enumerate all MSCs you want the OS to see.
2. For each MSC, describe:
   - How to reach it (MMIO vs PCC, base address, size).
   - How to handle its interrupts.
   - How many MPAM resources it contains and where those resources live in the system.

### 1.2 PCC (Platform Communication Channel)

PCC is a generic mechanism for **host–firmware shared-memory communication**, described in the **ACPI PCCT**. MPAM uses PCC as one of the possible **interface types** for an MSC:

- MPAM MSC Interface Type:
  - `0x00` – MMIO (SystemMemory)
  - `0x0A` – PCC

If you choose PCC for an MSC:

- The MPAM MSC node’s **Base Address** field holds the **PCC subspace ID**, *not* an MMIO address.
- The actual shared memory region and doorbell mechanisms are defined in the **PCCT subspace** referenced by that ID.

In ACS, PCC configuration macros describe those PCCT subspaces and the related doorbell and completion registers.

---

## 2. MPAM Platform Configuration

We’ll use the RD-N2-style macros as a reference and then generalize.

### 2.1 High-Level Steps for MPAM

For a new platform, go through this sequence:

1. **Enumerate all MSCs in the SoC**  
   Candidates include:
   - L1/L2/L3 caches and cluster-level caches.
   - Memory-side caches.
   - Memory controllers and channels (for memory bandwidth).
   - SMMUs (IO TLBs, translation caches in the SMMU datapath).
   - Inter-socket / inter-NUMA interconnects.
   - SoC interconnect buffers / fabrics that implement MPAM.
   - Other ACPI devices with MPAM-capable resources (implementation-specific).

2. **Decide interface type for each MSC**  
   - **MMIO**: registers are in normal memory-mapped region.
   - **PCC**: registers are read/written via a PCC shared memory region and doorbell.

3. **Define MSC-level properties**  
   For each MSC:
   - Assign an **Identifier** (must be unique across all MSCs).
   - Define access information (base address or PCC subspace ID, MMIO size).
   - Configure Overflow/Error interrupts and their affinities if implemented.
   - Set `MAX_NRDY_USEC` to match the worst-case “Not-Ready” time after configuration changes.
   - Optionally link the MSC to a device (HID/UID) for power management.

4. **Define resources inside each MSC**  
   - For each logical resource that the MSC controls (cache, bandwidth, interconnect link, etc), create a **resource node**.
   - Choose a **locator type** (processor cache, memory, SMMU, memory-side cache, ACPI device, interconnect, or unknown).
   - Fill in the locator descriptors for that type.
   - Decide RIS indices if the MSC supports RIS.

5. **Fill in ACS platform macros** based on steps 3–4:
   - `MPAM_MAX_MSC_NODE`, `MPAM_MAX_RSRC_NODE`, `PLATFORM_MPAM_MSC_COUNT`
   - MSCi config (ID, base, size, interrupts, MAX_NRDY, resource count)
   - Per-resource config (RIS index, locator type, descriptor fields).

### 2.2 Mapping to ACS-Style MPAM Macros

Using your RD-N2 example as a template:

```c
/* MPAM Config */
#define MPAM_MAX_MSC_NODE                     0x1
#define MPAM_MAX_RSRC_NODE                    0x1
#define PLATFORM_MPAM_MSC_COUNT               0x1

#define PLATFORM_MPAM_MSC0_INTR_TYPE          0x0
#define PLATFORM_MPAM_MSC0_ID                 0x3
#define PLATFORM_MPAM_MSC0_BASE_ADDR          0x1010028000
#define PLATFORM_MPAM_MSC0_ADDR_LEN           0x2004
#define PLATFORM_MPAM_MSC0_MAX_NRDY           10000000
#define PLATFORM_MPAM_MSC0_RSRC_COUNT         0x1

#define PLATFORM_MPAM_MSC0_RSRC0_RIS_INDEX    0x0
#define PLATFORM_MPAM_MSC0_RSRC0_LOCATOR_TYPE 0x1
#define PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR1  0x0
#define PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR2  0x0
```

You can think of this as:

- **Global limits:**
  - `MPAM_MAX_MSC_NODE` – maximum MSCs the override infrastructure can hold.
  - `MPAM_MAX_RSRC_NODE` – maximum resource nodes it can hold.
  - `PLATFORM_MPAM_MSC_COUNT` – how many MSCs are actually described for this platform.

- **Per-MSC macros:**
  - `PLATFORM_MPAM_MSCo_ID` → MSC node *Identifier* (must match MSC Device _UID if you expose one in ACPI).
  - `PLATFORM_MPAM_MSCo_BASE_ADDR` →
    - If Interface Type = MMIO: base of MSC’s MPAM register space.
    - If Interface Type = PCC: PCC subspace ID (the ACS code must know to interpret it that way).
  - `PLATFORM_MPAM_MSCo_ADDR_LEN` →
    - If MMIO: size of the MMIO region for MPAM registers.
    - If PCC: 0.
  - `PLATFORM_MPAM_MSCo_INTR_TYPE` → describes whether you use wired interrupts, MSI, or none (exact encoding is ACS-specific, typically 0 means wired/GSIV or “not used”).
  - `PLATFORM_MPAM_MSCo_MAX_NRDY` → value for `MAX_NRDY_USEC` in MSC node.
  - `PLATFORM_MPAM_MSCo_RSRC_COUNT` → number of resource nodes attached to this MSC.

- **Per-resource macros for MSC `o`, resource `r`:**
  - `PLATFORM_MPAM_MSCo_RSRCr_RIS_INDEX` → RIS index (0 if RIS not used).
  - `PLATFORM_MPAM_MSCo_RSRCr_LOCATOR_TYPE` → value from the location types table:
    - `0x00` – Processor cache
    - `0x01` – Memory
    - `0x02` – SMMU
    - `0x03` – Memory-side cache
    - `0x04` – ACPI device
    - `0x05` – Interconnect
    - `0xFF` – Unknown
  - `PLATFORM_MPAM_MSCo_RSRCr_DESCRIPTOR1` / `_DESCRIPTOR2` → the two parts of the locator structure (Descriptor1 is 8 bytes, Descriptor2 is 4 bytes). In ACS macros they are split into two values; how you pack them is implementation-specific, but conceptually:
    - Descriptor1 → “primary” identifier (cache ID, proximity domain, IORT identifier, etc).
    - Descriptor2 → “secondary” identifier (often 0 or reserved).

### 2.3 Choosing Locator Types and Descriptors

The locator type tells the OS *where* this resource lives. You choose it based on the component:

- **Processor cache (0x00)**  
  - Use for L1/L2/L3 caches.  
  - `Descriptor1` must match the Identifier of the PPTT Type 1 cache structure representing that cache.
  - `Descriptor2` is reserved / 0.

- **Memory (0x01)**  
  - Use for memory bandwidth resources.  
  - `Descriptor1` = SRAT proximity domain associated with the memory range.  
  - `Descriptor2` = 0.

- **SMMU (0x02)**  
  - Use for IO TLBs or translation caches in an SMMU.  
  - `Descriptor1` = Identifier of the IORT node describing that SMMU interface.  
  - `Descriptor2` = 0.

- **Memory-side cache (0x03)**  
  - Use for memory-side caches that sit in front of far memory.  
  - Descriptor encodes: `{Proximity domain, cache level}` (exact packing is implementation-specific in ACS; logically it’s that tuple).

- **ACPI device (0x04)**  
  - Use for implementation-specific resources associated with an ACPI-described device.  
  - `Descriptor1` = encoded ACPI _HID (or pointer to it, depending on your implementation).  
  - `Descriptor2` = _UID.

- **Interconnect (0x05)**  
  - Use for NUMA or processor-cluster interconnect links.  
  - `Descriptor1` often points to a resource-specific data block containing an interconnect descriptor table (UUID, number of descriptors, then an array of {source ID, dest ID, link type}).

- **Unknown (0xFF)**  
  - Use when the resource is in a component that cannot be described via ACPI, e.g. internal fabric buffers.

How you pack `Descriptor1` / `Descriptor2` into the ACS macros for each locator type is a pure implementation detail in the ACS platform override. The key is that they match the identifiers that the OS will see in PPTT, SRAT, HMAT, IORT and the ACPI namespace.

### 2.4 RIS Index

If your MSC supports RIS (Resource Instance Selection), then:

- `PLATFORM_MPAM_MSCo_RSRCr_RIS_INDEX` must be between 0 and `MPAMF_IDR.RIS_MAX` for that MSC.
- Each resource instance of the same type (e.g., multiple channels, multiple cache slices) gets a distinct RIS index.
- If the MSC does **not** support RIS (`HAS_RIS = 0`), then:
  - Set RIS index to 0 in all resource nodes.
  - All controls of a given type operate on that single resource instance.

### 2.5 Interrupts, MAX_NRDY, and Linked Devices

Per MSC node, you also need to decide:

- **Overflow interrupt / Error interrupt**:
  - GSIV numbers (wired) for overflow/error interrupts, or 0 if not present.
  - Interrupt flags (level/edge, processor vs processor container affinity).
  - Affinity (ACPI UID of the CPU or cluster that handles the interrupt).

- **`MAX_NRDY_USEC`**:
  - Worst-case time in microseconds for “Not Ready” to be deasserted after updating configuration.

- **Linked device (HID/UID)** (optional but recommended):
  - If MSC shares power management with some other ACPI device (CPU, cluster, memory controller, etc.), set:
    - Linked device HID = _HID of that device.
    - Linked device UID = _UID of that device.
  - This lets OSPM coordinate power/pstate for the MSC and its associated device.

In ACS macros, you might have extra fields for interrupt type and affinities; fill them consistently with the MPAM spec fields you intend to expose.

### 2.6 Example: Simple Memory Bandwidth MSC

Assume a new platform with:

- A single MPAM MSC (ID=3) at MMIO base `0x1010_028000`, size `0x2004`.
- The MSC controls **memory bandwidth** for the entire memory in proximity domain 0.
- No overflow/error interrupts.
- No RIS (single resource instance).

Then you could write:

```c
#define MPAM_MAX_MSC_NODE                     0x1
#define MPAM_MAX_RSRC_NODE                    0x1
#define PLATFORM_MPAM_MSC_COUNT               0x1

#define PLATFORM_MPAM_MSC0_INTR_TYPE          0x0      // e.g. 0 = no special handling / wired + none
#define PLATFORM_MPAM_MSC0_ID                 0x3
#define PLATFORM_MPAM_MSC0_BASE_ADDR          0x1010028000ULL
#define PLATFORM_MPAM_MSC0_ADDR_LEN           0x2004
#define PLATFORM_MPAM_MSC0_MAX_NRDY           10000000 // 10ms worst-case
#define PLATFORM_MPAM_MSC0_RSRC_COUNT         0x1

/* Resource 0: memory bandwidth for proximity domain 0 */
#define PLATFORM_MPAM_MSC0_RSRC0_RIS_INDEX    0x0      // no RIS
#define PLATFORM_MPAM_MSC0_RSRC0_LOCATOR_TYPE 0x1      // Memory
#define PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR1  0x0      // SRAT proximity domain 0
#define PLATFORM_MPAM_MSC0_RSRC0_DESCRIPTOR2  0x0      // reserved
```

Adapting this to a more complex topology just means adding more MSCs and more resources with appropriate locator types.

---

## 3. PCC Platform Configuration

### 3.1 When Do You Need PCC Config?

You only need to fully populate PCC-related platform macros when:

- At least one MPAM MSC (or some other firmware interface) uses **PCC** (Interface Type = PCC / ASID=0x0A), **and**
- The ACS tests expect to reach that MSC through a PCC subspace defined in PCCT.

If your MPAM MSCs are all MMIO-based, PCC configuration can stay as placeholders (or be unused).

### 3.2 Relationship Between MPAM and PCC

For an MPAM MSC with **PCC interface**:

- In the MSC node:
  - `Interface type` = PCC (0x0A).
  - `Base address` = the **subspace ID** of the PCC subspace in PCCT, not a physical address.
  - `MMIO size` = 0.
- In PCCT (PCC table):
  - There is a subspace of **type 1 (HW-reduced communications subspace)**.
  - That subspace references:
    - A **shared memory region** (“Generic Communication Shared Memory Region”), where the MPAM register image lives.
    - Optional **doorbell register(s)** and associated masks.
  - The MPAM feature page is mapped within this shared region at offset 8, after the PCC header fields.

In ACS macros, the PCC side is configured with something like:

```c
#define PLATFORM_PCC_SUBSPACE_COUNT                  0x1
#define PLATFORM_PCC_SUBSPACE0_INDEX                 0x0
#define PLATFORM_PCC_SUBSPACE0_TYPE                  0x1   // typically HW-reduced subspace
#define PLATFORM_PCC_SUBSPACE0_BASE                  <shared_mem_base>
#define PLATFORM_PCC_SUBSPACE0_MIN_REQ_TURN_TIME     <min turnaround in usec>
// doorbell / status configuration
#define PLATFORM_PCC_SUBSPACE0_DOORBELL_PRESERVE     <mask_of_bits_to_preserve>
#define PLATFORM_PCC_SUBSPACE0_DOORBELL_WRITE        <bits_to_set_when_ringing>
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_CHK_MASK <mask_for_completion_bit>
#define PLATFORM_PCC_SUBSPACE0_CMD_UPDATE_PRESERVE   <mask_preserved_on_update>
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_UPDATE_SET <bits_set_to_ack_completion>
/* GAS-style doorbell and status registers */
#define PLATFORM_PCC_SUBSPACE0_DOORBELL_REG          {space_id, bit_width, bit_offset, access_size, address}
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_UPDATE_REG { ... }
#define PLATFORM_PCC_SUBSPACE0_CMD_COMPLETE_CHK_REG  { ... }
```

(RD-N2 example uses dummy values, which is fine if MPAM is MMIO-only.)

### 3.3 Filling PCC Subspace Macros for a New Platform

For each PCC subspace you actually use:

1. **Decide subspace index and type**
   - `PLATFORM_PCC_SUBSPACEi_INDEX` → index used in your firmware/PCCT.
   - `PLATFORM_PCC_SUBSPACEi_TYPE` → typically `1` for HW-reduced communications subspace.
   - `PLATFORM_PCC_SUBSPACE_COUNT` → total number of subspaces you want to describe in ACS override.

2. **Shared memory region (BASE)**
   - `PLATFORM_PCC_SUBSPACEi_BASE` → base physical address of the PCC shared memory region used by this subspace.
   - Ensure the memory attributes are correct:
     - UEFI: set in EFI memory map.
     - Non-UEFI: must be mapped as Device-nGnRnE.

3. **Doorbell register**
   - Identify how software tells firmware that the command is ready:
     - Typically a register (GAS) whose write triggers an interrupt or wakeup.
   - Fill **Generic Address Structure** fields:
     - `space_id` – usually system memory or system I/O.
     - `bit_width`, `bit_offset`, `access_size` – describe the doorbell field.
     - `address` – base address of doorbell register.

   Example (simple memory-mapped doorbell bit 0 at address 0x2_0000_0000):

   ```c
   #define PLATFORM_PCC_SUBSPACE0_DOORBELL_REG {0x00, 32, 0, 3, 0x0000000200000000ULL}
   #define PLATFORM_PCC_SUBSPACE0_DOORBELL_PRESERVE 0xFFFFFFFEU  // preserve bits [31:1]
   #define PLATFORM_PCC_SUBSPACE0_DOORBELL_WRITE    0x00000001U  // set bit 0
   ```

4. **Command complete check + update registers**
   - Identify which status bit firmware sets when the command is complete and which register it lives in.
   - `PLATFORM_PCC_SUBSPACEi_CMD_COMPLETE_CHK_REG` → GAS for that status register.
   - `PLATFORM_PCC_SUBSPACEi_CMD_COMPLETE_CHK_MASK` → bit mask evaluated by the driver to know when firmware is done.
   - `PLATFORM_PCC_SUBSPACEi_CMD_COMPLETE_UPDATE_REG` → register written by the OS to clear/ack the completion.
   - `PLATFORM_PCC_SUBSPACEi_CMD_COMPLETE_UPDATE_SET` → bits to write to clear the completion bit.
   - `PLATFORM_PCC_SUBSPACEi_CMD_UPDATE_PRESERVE` → mask of bits preserved when writing the update/ack.

   If your PCC implementation uses the standard ACPI `Command`/`Status` semantics in the shared memory region, these macros simply reflect that mapping.

5. **Minimum request turnaround time**
   - `PLATFORM_PCC_SUBSPACEi_MIN_REQ_TURN_TIME` → minimum time (in microseconds) between PCC requests.
   - This should match the PCCT and your firmware’s expectations.

6. **Hooking PCC into MPAM MSC nodes**
   - For every MSC that uses this PCC subspace:
     - Set its **Interface type** to PCC.
     - Set its **Base address** field to the PCC subspace index.
     - Set its `MMIO size` field to 0.

   In your ACS macros, this means making sure that the MPAM MSC macros and the PCC subspace macros agree on which subspace index is used.

### 3.4 When PCC Macros Can Stay as Placeholders

If your platform:

- Implements all MPAM MSCs as MMIO only, and
- Does not use PCC for other ACS-tested channels,

then you can leave PCC macros as:

```c
#define PLATFORM_PCC_SUBSPACE_COUNT                  0x0
/* or a dummy 0th subspace with REGs set to 0 / DEADDEAD as in RD-N2 example */
```

---
