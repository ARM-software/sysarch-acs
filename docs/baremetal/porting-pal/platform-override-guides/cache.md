# Processor Topology & Cache Configuration Guide (Platform Override)

This guide explains how to populate **processor cache topology** configuration in the platform override header, using the **RD-N2 reference values** as a worked example.

---

## 1) What this configuration is used for

The platform override cache/topology data is used to describe:

- **Which caches exist** (L1I, L1D, L2, L3…)
- **How caches are chained** (e.g., L1 → L2 → end)
- **Which caches are private vs shared**
- **Cache identifiers (Cache IDs)** that allow the firmware/OS tooling to correlate “cache instances” to CPU nodes and sharing groups

In the RD-N2 example (see `pal/baremetal/target/RDN2/include/platform_override_fvp.h`), the cache database is built as a **set of cache descriptors** (`PLATFORM_CACHE*_*`) plus a **per-processor mapping** (`PLATFORM_PPTT*_*`) that selects the cache IDs for each processing element / leaf node.

> Key idea: you define caches once (with IDs and next-level links), then reference those IDs from CPU nodes.

---

## 2) High-level structure

Your override is split into two layers:

### A. Cache Descriptor List (the “cache database”)
A flat list of cache descriptors:
- `PLATFORM_OVERRIDE_CACHE_CNT`
- `PLATFORM_CACHE<n>_FLAGS`
- `PLATFORM_CACHE<n>_OFFSET`
- `PLATFORM_CACHE<n>_NEXT_LEVEL_INDEX`
- `PLATFORM_CACHE<n>_SIZE`
- `PLATFORM_CACHE<n>_CACHE_ID`
- `PLATFORM_CACHE<n>_IS_PRIVATE`
- `PLATFORM_CACHE<n>_TYPE`

These describe *what caches exist* and *how they link together*.

### B. Per-PE Cache Mapping (CPU → Cache IDs)
A list mapping each CPU leaf node to its cache IDs:
- `PLATFORM_PPTT<i>_CACHEID0`
- `PLATFORM_PPTT<i>_CACHEID1`

These describe *which cache IDs belong to each PE* (typically L1D and L1I, or other “head” caches). From those head caches, the **NEXT_LEVEL_INDEX** chain describes downstream caches (e.g., L2).

---

## 3) Fields in the cache descriptors

### 3.1 `PLATFORM_OVERRIDE_CACHE_CNT`
**Total number of cache descriptors** you define.

Example:
```c
#define PLATFORM_OVERRIDE_CACHE_CNT 0x30
```

This must be:
- ≥ the highest `PLATFORM_CACHE<n>` index you define + 1
- consistent with any internal array sizing assumptions in the platform override implementation

---

### 3.2 `PLATFORM_CACHE<n>_CACHE_ID`
A **non-zero unique ID** for the cache descriptor.

Example:
```c
#define PLATFORM_CACHE0_CACHE_ID 0x1
#define PLATFORM_CACHE2_CACHE_ID 0x2
#define PLATFORM_CACHE1_CACHE_ID 0x3
```

**Rules of thumb**
- Cache IDs must be globally unique. ACPI tables such as APMT or MPAM may reference caches by ID, so duplicates break cross-table linking and are not permitted.
- If you need **per-core cache identification**, give each cache instance its own descriptor and ID; never reuse a descriptor across multiple cores even if the caches are identical.

In RD-N2, you can see a pattern:
- CPU0 uses cache IDs `0x1` and `0x2`
- CPU1 uses `0x1001` and `0x1002`
- CPU2 uses `0x2001` and `0x2002`
…which implies **unique cache instances per CPU** for L1 caches (and likely unique L2 as well).

---

### 3.3 `PLATFORM_CACHE<n>_TYPE`
Cache “kind” (data, instruction, unified).

Examples in RD-N2:
```c
#define PLATFORM_CACHE0_TYPE 0   // Data
#define PLATFORM_CACHE2_TYPE 1   // Instruction
#define PLATFORM_CACHE1_TYPE 2   // Unified
```

Interpretation used in your reference:
- `0` = Data cache (L1D)
- `1` = Instruction cache (L1I)
- `2` = Unified cache (e.g., L2)

If your platform has separate L2I/L2D (rare on modern Arm servers), you’d model them as separate chains; otherwise, use unified for L2/L3.

---

### 3.4 `PLATFORM_CACHE<n>_SIZE`
Cache size in bytes.

Examples:
```c
#define PLATFORM_CACHE0_SIZE 0x10000    // 64 KB (L1D)
#define PLATFORM_CACHE2_SIZE 0x10000    // 64 KB (L1I)
#define PLATFORM_CACHE1_SIZE 0x100000   // 1 MB (L2)
```

Populate from:
- SoC TRM / core integration guide
- actual CPU cache registers (if you can probe) during bring-up (and then freeze into override)

---

### 3.5 `PLATFORM_CACHE<n>_NEXT_LEVEL_INDEX`
Defines the “linked list” relationship to the **next cache level**.

Examples (CPU0 chain):
```c
#define PLATFORM_CACHE0_NEXT_LEVEL_INDEX 1   // L1D -> cache1 (L2)
#define PLATFORM_CACHE2_NEXT_LEVEL_INDEX 1   // L1I -> cache1 (L2)
#define PLATFORM_CACHE1_NEXT_LEVEL_INDEX -1  // L2 -> end
```

**How to use**
- For L1 caches, point to the L2 cache descriptor index.
- For L2, point to L3 if it is private to the same node; otherwise `-1` if it ends there.
- Use `-1` (or the implementation’s “null” marker) to terminate.

> If your system has a shared L3 at a cluster/package level, represent L3 as a separate descriptor and ensure the chain points to it at the right level (depending on how your consuming code models “shared”).

---

### 3.6 `PLATFORM_CACHE<n>_IS_PRIVATE`
Indicates whether the cache is private to the associated CPU node.

Example:
```c
#define PLATFORM_CACHE1_IS_PRIVATE 0x1
```

In RD-N2, all shown caches are marked private (`0x1`). That suggests:
- either the model only describes *per-CPU private caches*, or
- shared caches are modeled differently elsewhere, or
- sharing is not expressed in this particular override format.

**For a new platform**
- If you have shared L3, decide how your platform override expects you to represent it:
  - Option A: shared cache descriptor referenced by multiple nodes, `IS_PRIVATE = 0`
  - Option B: per-cluster cache descriptor referenced by a “cluster node” (if your format supports non-leaf nodes)
  - Option C: not modeled here (and discovered elsewhere), in which case keep private caches only

---

### 3.7 `PLATFORM_CACHE<n>_FLAGS`
A bitfield indicating which cache properties are valid in the descriptor.

RD-N2 uses:
```c
#define PLATFORM_CACHE<n>_FLAGS 0xFF
```

Meaning: “all relevant properties are valid and should be used”.

For new platforms:
- `0xFF` is a safe baseline if your code expects explicit properties.
- If your consuming stack can discover some properties dynamically, you *could* reduce flags, but that is typically not needed unless required.

---

### 3.8 `PLATFORM_CACHE<n>_OFFSET`
This is an implementation detail used by the firmware generator/consumer as a reference into a serialized structure blob (or to compute relative pointers).

Example:
```c
#define PLATFORM_CACHE0_OFFSET 0x68
#define PLATFORM_CACHE1_OFFSET 0xA0
...
```

**Important:** These offsets are **not arbitrary**.
They usually must match:
- the byte offset within the built structure blob, or
- a specific packing layout expected by the codebase.

**For a new platform**
- Do not “invent” offsets.
- Derive them the same way RD-N2 does:
  - either by using the same macro generator logic (recommended),
  - or by following the same layout rules (size of each structure, alignment constraints).
- If your project has a generator C file that emits the structures, ensure it computes these offsets consistently.

If you don’t have a generator, a typical pattern is:
- cache descriptor structures are fixed size (e.g., 28 bytes),
- each entry offset increments by that size (plus alignment),
but you should confirm against your actual code.

---

## 4) Per-processor cache mapping (`PLATFORM_PPTT<i>_CACHEID*`)

These macros associate each CPU leaf node with the “head” cache IDs it owns.

Example:
```c
#define PLATFORM_PPTT0_CACHEID0 0x1
#define PLATFORM_PPTT0_CACHEID1 0x2
```

In RD-N2, the pattern repeats per CPU:
- `CACHEID0` looks like **L1D**
- `CACHEID1` looks like **L1I**

For CPU1:
```c
#define PLATFORM_PPTT1_CACHEID0 0x1001
#define PLATFORM_PPTT1_CACHEID1 0x1002
```

This implies:
- each CPU has distinct L1 caches (different IDs),
- and each CPU’s L1 caches point to its L2 via the descriptor chain.

### How to populate for a new platform
1. Decide how many CPU leaf nodes you have (matches your PE count).
2. For each CPU `i`, assign:
   - `CACHEID0` → its data-cache head (usually L1D)
   - `CACHEID1` → its instruction-cache head (usually L1I)
3. Ensure those cache IDs exist in the cache descriptor list.
4. Ensure each of those descriptors links correctly via `NEXT_LEVEL_INDEX`.

---

## 5) Reading the RD-N2 example as a template

### CPU0 caches
Descriptors:
- `CACHE0` (ID 0x1, Data, 64KB) → next level = index 1
- `CACHE2` (ID 0x2, Instr, 64KB) → next level = index 1
- `CACHE1` (ID 0x3, Unified, 1MB) → next level = -1

CPU0 mapping:
```c
PLATFORM_PPTT0_CACHEID0 = 0x1   // L1D head
PLATFORM_PPTT0_CACHEID1 = 0x2   // L1I head
```

This implies an L1D/L1I pair feeding a private L2.

### CPU1 caches
Descriptors:
- `CACHE3` (ID 0x1001) → next level = index 4
- `CACHE5` (ID 0x1002) → next level = index 4
- `CACHE4` (ID 0x1003) → next level = -1

CPU1 mapping:
```c
PLATFORM_PPTT1_CACHEID0 = 0x1001
PLATFORM_PPTT1_CACHEID1 = 0x1002
```

…and so on for each CPU.

---

## 6) What to change for a new platform

### 6.1 If the CPU count differs
- Update the number of per-CPU mapping entries (`PLATFORM_PPTT<i>_*` count).
- Decide whether to keep **unique caches per CPU** (like RD-N2) or share identical structures.

### 6.2 If cache sizes differ
- Update `*_SIZE` for the corresponding descriptors.

### 6.3 If cache topology differs
Examples:
- **Shared L3 per cluster**: you may want L2 → L3 and L3 → end, and reference the same L3 descriptor from all CPUs in the cluster.
- **No separate L2 (rare)**: L1 → end.
- **L2 shared across a pair**: point both CPUs’ L1 heads to the same L2 descriptor.

The key is to keep the `NEXT_LEVEL_INDEX` chain consistent with the topology you want represented.

### 6.4 Offsets must follow your implementation rules
If your platform generator auto-computes offsets, you should not hand-edit them.
If you must edit them, ensure:
- correct structure sizes
- correct alignment
- monotonic increasing offsets
- no collisions

---

## 7) Minimal skeleton for a new platform (template)

```c
/* Cache descriptor count */
#define PLATFORM_OVERRIDE_CACHE_CNT <N>

/* Cache descriptors */
#define PLATFORM_CACHE0_FLAGS            0xFF
#define PLATFORM_CACHE0_OFFSET           <computed>
#define PLATFORM_CACHE0_NEXT_LEVEL_INDEX <idx_or_-1>
#define PLATFORM_CACHE0_SIZE             <bytes>
#define PLATFORM_CACHE0_CACHE_ID         <unique_nonzero>
#define PLATFORM_CACHE0_IS_PRIVATE       0x1
#define PLATFORM_CACHE0_TYPE             0 /* Data */

/* ... other cache descriptors ... */

/* Per-CPU cache mapping (heads) */
#define PLATFORM_PPTT0_CACHEID0 <L1D_ID_CPU0>
#define PLATFORM_PPTT0_CACHEID1 <L1I_ID_CPU0>
#define PLATFORM_PPTT1_CACHEID0 <L1D_ID_CPU1>
#define PLATFORM_PPTT1_CACHEID1 <L1I_ID_CPU1>
/* ... */
```

---
