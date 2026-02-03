# Memory attributes & bandwidth hints — platform override guide

This document describes how to fill the **platform override configuration** for:

- The number of **memory proximity domains** you want to describe
- A list of **memory domain entries**
- Per-domain **read/write peak bandwidth hints**
- The **common encoding** for the bandwidth dataset (data type, base unit, flags)

The intent is to provide software with a consistent, complete set of *initiator→memory-domain* performance hints (here: bandwidth) for memory placement and optimization.

---

## 1) What you need from the platform

Collect the following before you set values:

1. **Memory proximity domain IDs**
   - A small integer per memory domain (0..N-1 is typical).
   - These IDs must match whatever the platform uses to describe memory/NUMA domains elsewhere (e.g., SRAT/firmware topology).

2. **Peak sustainable bandwidth per memory domain**
   - Provide **read** and **write** numbers **per domain**, in a *normalized* representation.
   - You can use either:
     - Measured values (recommended), or
     - Spec/SoC interconnect limits (acceptable for early bring-up)

3. **A base unit** for bandwidth entries
   - The base unit defines how to interpret the per-domain “entry” value.
   - Represented bandwidth = `entry_value × entry_base_unit`.

---

## 2) Macro groups and what they mean

### A. Top-level counts

```c
#define PLATFORM_OVERRIDE_NUM_OF_HMAT_PROX_DOMAIN  1
#define PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES         0x4
```

- `PLATFORM_OVERRIDE_NUM_OF_HMAT_PROX_DOMAIN`
  - Number of *initiator proximity domains* described in this dataset.
  - In many server designs, you can start with **1** initiator domain representing “the CPUs / initiators as a group”.
  - Increase this if you want to publish different bandwidth numbers depending on which initiator domain is accessing memory.

- `PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES`
  - Number of **memory domain entries** you will populate (i.e., how many memory proximity domains you are publishing bandwidth for).
  - Must match the number of `PLATFORM_HMAT_MEMx_*` blocks you define.


### B. Dataset description (bandwidth encoding)

```c
#define HMAT_NODE_MEM_SLLBIC                  0x1
#define HMAT_NODE_MEM_SLLBIC_DATA_TYPE        0x3
#define HMAT_NODE_MEM_SLLBIC_FLAGS            0x0
#define HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT  0x64
```

These macros describe the **kind of dataset** you’re publishing and how to interpret entry values.

- `HMAT_NODE_MEM_SLLBIC`
  - Selects the dataset category used by the implementation (commonly: a “system locality latency/bandwidth” dataset selector).
  - In this reference, it is set to `0x1` indicating the **latency/bandwidth dataset** is enabled/selected.

- `HMAT_NODE_MEM_SLLBIC_DATA_TYPE`
  - Selects the **data type** within the dataset.
  - In this reference, `0x3` means **Access Bandwidth** (i.e., a single bandwidth number used for both reads and writes *if they are the same*).
  - However, your per-domain macros provide **separate read and write values**, which is also a common platform practice; treat them as read/write bandwidth entries even if the dataset is “access bandwidth”.
  - If your firmware implementation supports explicit **Read Bandwidth** / **Write Bandwidth** types, prefer those when read≠write.

- `HMAT_NODE_MEM_SLLBIC_FLAGS`
  - Qualifiers for the dataset.
  - `0x0` typically means **default**: no special access attribute qualifiers (e.g., not indicating “non-sequential” or “minimum transfer size”).

- `HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT`
  - The scaling factor for each entry.
  - For bandwidth datasets, interpret as **MB/s per unit**.
  - With `0x64` (decimal 100): an entry value of `0x82` (130) corresponds to `130 × 100 = 13,000 MB/s`.

---

## 3) Per-memory-domain entries

Each memory domain entry provides:

- The **memory proximity domain ID**
- A **max write bandwidth** entry
- A **max read bandwidth** entry

Reference pattern:

```c
#define PLATFORM_HMAT_MEMx_PROX_DOMAIN        <domain_id>
#define PLATFORM_HMAT_MEMx_MAX_WRITE_BW       <write_entry>
#define PLATFORM_HMAT_MEMx_MAX_READ_BW        <read_entry>
```

Where the represented bandwidth is:

- `MaxWriteBandwidth = PLATFORM_HMAT_MEMx_MAX_WRITE_BW × HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT` (MB/s)
- `MaxReadBandwidth  = PLATFORM_HMAT_MEMx_MAX_READ_BW  × HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT` (MB/s)


### RD N2 example (as provided)

```c
#define PLATFORM_OVERRIDE_NUM_OF_HMAT_PROX_DOMAIN 1
#define PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES    0x4

#define HMAT_NODE_MEM_SLLBIC                  0x1
#define HMAT_NODE_MEM_SLLBIC_DATA_TYPE        0x3
#define HMAT_NODE_MEM_SLLBIC_FLAGS            0x0
#define HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT  0x64

#define PLATFORM_HMAT_MEM0_PROX_DOMAIN        0x0
#define PLATFORM_HMAT_MEM0_MAX_WRITE_BW       0x82
#define PLATFORM_HMAT_MEM0_MAX_READ_BW        0x82

#define PLATFORM_HMAT_MEM1_PROX_DOMAIN        0x1
#define PLATFORM_HMAT_MEM1_MAX_WRITE_BW       0x8c
#define PLATFORM_HMAT_MEM1_MAX_READ_BW        0x8c

#define PLATFORM_HMAT_MEM2_PROX_DOMAIN        0x2
#define PLATFORM_HMAT_MEM2_MAX_WRITE_BW       0x96
#define PLATFORM_HMAT_MEM2_MAX_READ_BW        0x96

#define PLATFORM_HMAT_MEM3_PROX_DOMAIN        0x3
#define PLATFORM_HMAT_MEM3_MAX_WRITE_BW       0xa0
#define PLATFORM_HMAT_MEM3_MAX_READ_BW        0xa0
```

#### What these numbers represent

With `ENTRY_BASE_UNIT = 0x64 = 100 MB/s`:

| Memory domain | Entry (read/write) | Represented bandwidth |
|---:|---:|---:|
| 0 | 0x82 = 130 | 130 × 100 = **13,000 MB/s** (~13.0 GB/s) |
| 1 | 0x8C = 140 | 140 × 100 = **14,000 MB/s** (~14.0 GB/s) |
| 2 | 0x96 = 150 | 150 × 100 = **15,000 MB/s** (~15.0 GB/s) |
| 3 | 0xA0 = 160 | 160 × 100 = **16,000 MB/s** (~16.0 GB/s) |

Because read and write entries are equal in this reference, this is consistent with “Access Bandwidth”.

---

## 4) Choosing good values

### A. Picking `ENTRY_BASE_UNIT`

Choose a base unit that:

- Preserves ordering (higher entry ⇒ higher bandwidth)
- Avoids overflow/saturation in the firmware data structures
- Keeps entry values reasonably sized (e.g., 10–10,000, not 1–2)

Typical options:

- `100 MB/s` (0x64) — good for 10–200 GB/s platforms
- `1000 MB/s` (0x3E8) — good for very high bandwidth fabrics
- `10 MB/s` (0x0A) — if you need finer granularity

### B. Deriving entry values from measured bandwidth

If you measure peak bandwidth in GB/s:

1. Convert to MB/s: `GB/s × 1024` (or ×1000 if your measurement tool reports decimal GB)
2. Compute entry: `entry = round(MB/s / ENTRY_BASE_UNIT)`
3. Clamp to the supported range of your implementation (commonly 16-bit entries)

Example: target 51.2 GB/s (decimal) ≈ 51,200 MB/s with base unit 100 MB/s:

- `entry ≈ 512` (`0x200`)

---

## 5) Completeness and consistency rules

Use this checklist to avoid common integration failures:

- **Counts match reality**
  - `PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES` equals the number of `PLATFORM_HMAT_MEMx_*` blocks.

- **Domain IDs are consistent**
  - Each `*_PROX_DOMAIN` value must match the platform’s memory domain numbering.
  - Do not reuse a proximity domain ID across two entries.

- **Dataset is complete for the chosen shape**
  - If you publish bandwidth for a set of initiators and targets, you must publish entries for all relevant initiator→target pairs as required by your firmware’s HMAT construction logic.
  - In this simplified RD N2 style (single initiator domain), ensure you provide bandwidth numbers for **all memory domains**.

- **Read vs write semantics**
  - If read and write are materially different on your platform, prefer separate read/write dataset types (if supported).
  - Otherwise, keep them equal as in the reference.

- **Units are not ambiguous**
  - Document the base unit you choose and confirm consumers interpret it as MB/s.

---

## 6) Quick template for a new platform

```c
/* How many initiator proximity domains are described (often 1) */
#define PLATFORM_OVERRIDE_NUM_OF_HMAT_PROX_DOMAIN <num_initiators>

/* How many memory proximity domains are described */
#define PLATFORM_OVERRIDE_HMAT_MEM_ENTRIES        <num_memory_domains>

/* Dataset selector + encoding */
#define HMAT_NODE_MEM_SLLBIC                  0x1
#define HMAT_NODE_MEM_SLLBIC_DATA_TYPE        <bandwidth_type>
#define HMAT_NODE_MEM_SLLBIC_FLAGS            0x0
#define HMAT_NODE_MEM_SLLBIC_ENTRY_BASE_UNIT  <mb_per_s_per_unit>

/* Per memory domain entries */
#define PLATFORM_HMAT_MEM0_PROX_DOMAIN        <mem_dom_0>
#define PLATFORM_HMAT_MEM0_MAX_WRITE_BW       <entry_write_0>
#define PLATFORM_HMAT_MEM0_MAX_READ_BW        <entry_read_0>

/* ... repeat for MEM1..MEM(n-1) */
```

---
