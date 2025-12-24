# RAS2 Platform Override Configuration Guide (for new platforms)

This guide explains how to populate **RAS2** platform override macros for a new platform.

RAS2 provides a scalable way for the OS (OSPM) to discover and control platform RAS features **per component instance** (for example, per NUMA proximity domain for memory). The platform supports **either RAS2 or RASF, not both**.

---

## 1) What RAS2 represents at a high level

RAS2 is an ACPI table that lists **RAS2 PCC descriptors**. Each descriptor ties:
- a **PCC subspace** (defined in **PCCT**) to
- a **RAS feature type** (e.g., Memory) and
- an **Instance identifier** (e.g., proximity domain for memory).

### Key linkage
- RAS2 **PCC Identifier** → indexes into the **PCCT subspace array**
- RAS2 **Feature Type** → what class of RAS feature this subspace controls (Memory = `0x00`)
- RAS2 **Instance** → which component instance it applies to
  - For **Memory RAS features**, Instance **must match the SRAT proximity domain**

So: **SRAT defines the proximity domains**, and RAS2 uses those IDs to provide **per-domain** RAS controls (scrubbing, address translation, etc.).

---

## 2) How RAS2 maps to the platform override macros

Override uses “blocks” that conceptually represent RAS2 descriptors (typically one per instance).

```c
#define RAS2_MAX_NUM_BLOCKS                   0x4
#define PLATFORM_OVERRIDE_NUM_RAS2_BLOCK      0x3
#define PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK  0x3
```

### Macro meaning
- `RAS2_MAX_NUM_BLOCKS`
  - Upper bound supported by your override implementation
- `PLATFORM_OVERRIDE_NUM_RAS2_BLOCK`
  - How many RAS2 descriptors you will actually publish
- `PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK`
  - How many of those descriptors are for **Memory feature type (0x00)**

> If you later add vendor-defined feature types (0x80–0xFF), you’d track those similarly (if your override supports per-feature counts).

---

## 3) The core per-block fields to populate

Each RAS2 descriptor needs to encode the equivalent of:

- PCC Identifier (PCCT subspace index)
- Feature Type
- Instance

Override flattens this into macros like:

```c
#define PLATFORM_OVERRIDE_RAS2_BLOCK0_PROXIMITY             0x0
#define PLATFORM_OVERRIDE_RAS2_BLOCK0_PATROL_SCRUB_SUPPORT  0x1
```

### Interpretation for Memory feature type
For **Memory RAS features** (Feature Type `0x00`):
- **Instance = Proximity Domain**
- Proximity Domain must match SRAT memory proximity domain definitions

So the field `*_PROXIMITY` is the **RAS2 Instance** for feature type Memory.

---

## 4) How to fill RAS2 for a new platform (step-by-step)

### Step 1 — Decide whether you should use RAS2
Use RAS2 when:
- you want the OS to control RAS features via PCC, and/or
- you need per-instance scaling (e.g., per NUMA domain), and/or
- you want OS-managed memory scrubbing / LA→PA translation services.

Do **not** publish both RAS2 and RASF.

---

### Step 2 — Identify the RAS2 feature types you will support
The spec defines:
- `0x00` = Memory RAS features
- `0x01–0x7F` reserved
- `0x80–0xFF` vendor-defined

Most systems begin with **Memory (0x00)** since it’s explicitly defined and ties nicely to SRAT proximity domains.

---

### Step 3 — Determine the “instances” for each feature type

#### For Memory RAS features (Feature Type 0x00)
Instance **must be** the **SRAT proximity domain** (NUMA domain) for that memory.

Practical mapping approaches:
- **UMA system**: only one proximity domain (usually 0)
  - publish one RAS2 descriptor: Instance = 0
- **NUMA system**: multiple proximity domains
  - publish one RAS2 descriptor per domain you want manageable independently

**Example**
If SRAT defines memory proximity domains `{0,1,2}`, and you want independent scrub controls per domain:
- publish 3 RAS2 descriptors:
  - Instance 0
  - Instance 1
  - Instance 2

This matches RD-N2 sample.

---

### Step 4 — Choose / allocate PCC subspaces in PCCT
RAS2 does not define the PCC subspace itself — it references PCCT.

You must ensure:
- PCCT contains enough subspaces for your RAS2 descriptors
- Each descriptor’s PCC Identifier points to a valid PCCT subspace index
- Each subspace provides a shared memory region with the RAS2 communication layout

#### Practical guidance
- Most platforms assign **one PCC subspace per instance** (scales cleanly)
- If firmware/SoC only supports one mailbox but multiplexes internally, you *can* share subspaces, but you lose independent concurrency and may violate the “channel dedicated to a given component instance” intent.

**Rule of thumb:** prefer **dedicated PCC per instance** unless you have a strong reason not to.

---

### Step 5 — Decide what memory features you expose per instance
For Memory RAS features, the spec defines at least:
- `PATROL_SCRUB` (bit 0)
- `LA2PA_TRANSLATION` (bit 1)

Your override currently has:
```c
#define PLATFORM_OVERRIDE_RAS2_BLOCKn_PATROL_SCRUB_SUPPORT  0x1
```

So for a new platform you should decide per proximity domain whether:
- scrub engine exists
- scrub engine is controllable by OS
- you want to expose it (some platforms keep it firmware-managed)

#### Recommended bring-up sequence
1. Expose `PATROL_SCRUB` first (if supported)
2. Add `LA2PA_TRANSLATION` once you have correct component scoping and translation correctness

---

## 5) Recommended macro schema for a robust implementation

Sample shows only proximity + scrub support. For a production-quality override, a per-block set usually needs:

### A) Descriptor identity
- `BLOCKn_PCC_ID` (PCCT subspace index)
- `BLOCKn_FEATURE_TYPE` (0x00 for memory)
- `BLOCKn_INSTANCE` (proximity domain)

### B) Feature support bitmap (or per-feature macros)
- patrol scrub supported
- LA→PA supported
- other future bits

### C) Optional: capability constraints per instance
- min scrub rate
- max scrub rate
- alignment constraints for ranges
- maximum number of parameter blocks

---

## 6) Translating RD-N2 style into a new-platform filling rule

### Rule 1 — Count fields
- `PLATFORM_OVERRIDE_NUM_RAS2_BLOCK` = number of descriptors you will publish
- `PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK` = how many of those have Feature Type 0x00

### Rule 2 — Per-block proximity
For memory blocks:
- `PLATFORM_OVERRIDE_RAS2_BLOCKn_PROXIMITY` = SRAT proximity domain ID

### Rule 3 — Patrol scrub support
Set:
- `*_PATROL_SCRUB_SUPPORT = 1` if:
  - hardware supports scrubbing for that domain, and
  - platform is willing to let OS control it
Otherwise set it to 0.

### Rule 4 — Ensure PCCT is consistent
For each published block:
- there must be a corresponding PCC subspace in PCCT
- the OS must be able to perform “Execute RAS2 Command” on that subspace

If you publish 3 blocks but only 1 PCCT subspace, you must ensure the PCC identifier mapping is valid and multiplexing is correct (not recommended for independent control).

---

## 7) How the OS will use this (useful for validation)

OSPM workflow (simplified):
1. Parse RAS2 descriptors
2. For each descriptor, locate PCCT subspace via PCC Identifier
3. Read the RAS2 communication region to discover supported features (bitmap)
4. To invoke a feature:
   - set “Set RAS Capabilities” bitmap
   - fill the parameter block (e.g., PATROL_SCRUB structure)
   - issue PCC Execute command (`0x01`)

So validation for a new platform typically includes:
- OS can enumerate all instances
- feature bitmap matches what you claim
- parameter blocks work and return expected status
- scrub start/stop commands behave and are properly scoped per proximity domain
- LA→PA translation returns correct physical addresses (when supported)

---

## 9) Quick template: filling RAS2 for memory-only platforms

If your platform has N proximity domains with memory and supports patrol scrub on all:

- `PLATFORM_OVERRIDE_NUM_RAS2_BLOCK = N`
- `PLATFORM_OVERRIDE_NUM_RAS2_MEM_BLOCK = N`
- For each domain `d[i]`:
  - `BLOCKi_PROXIMITY = d[i]`
  - `BLOCKi_PATROL_SCRUB_SUPPORT = 1`

If only some domains support scrub:
- Set scrub support per-domain accordingly

---
