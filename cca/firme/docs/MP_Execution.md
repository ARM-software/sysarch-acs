# Multiprocessor execution

FIRME provides NS-EL2 worker PEs for explicitly MP-aware scenarios. The normal
rule dispatcher remains serial: this infrastructure does **not** run every
existing test simultaneously or claim additional rule coverage by itself.
MP-aware entries execute as part of the normal registry, not a separate run.

## Test-selected PE execution

`firme_test_case_t` supports `num_pe`, `mp_entry`, and `mp_payload`:

- Set `num_pe` to a fixed count or `VAL_TEST_ALL_PES`. Insufficient PEs produce
  an explicit SKIP; VAL does not silently reduce the requested count.
- The dispatcher calls `mp_entry(world, num_pe, run_payload, out)` for each
  declared caller world. The test owns that world's setup, payload phases,
  reduction and cleanup; VAL combines these into one rule result.
- Required-world unavailability is an error. A required-world SKIP prevents
  an overall PASS, and an optional world's malformed response is not an
  availability SKIP. Successful MP aggregation runs the same `post_run` hook
  as ordinary tests, with all per-world result words available.
- Invoke `run_payload(num_pe, &request, results)` for a phase. The request names
  the registered rule, caller world, phase, and a 32-bit test-defined argument.
  Results include the actual PE index and all four 64-bit rule-result words.
- VAL runs the controller first, then the requested number of other PEs from
  the platform affinity table. It owns CPU_ON, readiness, dispatch, wait and
  confirmed CPU_OFF. Every requested PE must complete; timeouts are not SKIPs.
- This convenience runner serializes payloads, including cross-world calls.
  Use the lower-level worker API for deliberately concurrent scenarios, with
  private contexts and explicit synchronization. Sequential enumeration is
  sufficient for hardware-width discovery, not concurrency-rule validation.
- Each world retains results per PE. Result reads transfer two 32-bit halves
  at a time, preserving negative status encodings and full 64-bit diagnostics
  despite SMC32 messaging and RMMD's limited return-register forwarding.
- Worker contexts reside in persistent VAL storage, not the entry's stack.
  Failed cleanup prevents storage reuse and stops subsequent suite execution.

Ordinary Realm rule calls also retain results per PE and retrieve every word
with a matching execution sequence. This preserves `last_x1` diagnostics without
assuming that the platform dispatcher forwards additional return registers.

R0239 registers `VAL_TEST_ALL_PES`. It reads MECIDR_EL2 on each PE in Realm,
reads every discovered SMMU's R_IDR3 and R_MECIDR, and computes the independent
minimum before checking DUT advertisements and exercising MECID boundaries
on every PE. The normal summary still counts this as one test.

## Platform contract

- `platform/pal_baremetal/targets/<target>/include/pal_pe.h` supplies
  `PAL_PE_COUNT`, `PAL_PE_MPIDRS` and the MPIDR affinity mask. `pal_pe.c`
  publishes the immutable affinity table. This follows the bare-metal ACS
  platform-table approach, rather than probing guessed CPU IDs.
- The FVP configuration describes two four-core clusters, one thread per core,
  with MPIDR.MT set. Aff0 is the thread, Aff1 the core and Aff2 the cluster.
  A changed model topology must have a matching PAL table. Unknown affinities
  stop before allocating a stack; duplicate entries fail initialization.
- Firmware must provide PSCI CPU_ON, CPU_OFF and level-zero AFFINITY_INFO using
  SMC64 where applicable. It must join PEs to coherency before payload entry
  and perform the architected power-down cache maintenance.
- NS shared data uses identical Normal WBWA, Inner Shareable mappings on every
  worker. Non-coherent platforms are not supported by this implementation.
  Cache lines must not exceed the 256-byte mailbox isolation boundary.
- Each payload reserves a loadable 32 KiB stack per PE. Linker capacity checks
  still apply. The supplied SPMC manifest describes the full 1 MiB Secure
  reservation, including secondary stacks.

The primary PE is identified at runtime; it need not have table index zero.
The PE table is not a SMMU inventory. The target's `pal_iovirt.c` separately
lists every SMMU's NS and Realm page-0 bases, following the bare-metal ACS
platform-table approach. Widths are read from hardware, never supplied by PAL
or copied from FIRME advertising. Validate the complete inventory when porting;
an inaccessible controller must not be dropped from the minimum calculation.
This is read-only SMMU discovery, not a stream-table/DMA driver or ACPI parser.

FVP uses NS base `0x2b400000`, Root page offset `0x20000`, and Realm page-0 base
`0x2b440000` (`BA_REALM=2`). The regression script configures these matching
windows whenever SMMU MEC is enabled. Partners must supply their actual Realm
addresses; ACS cannot read Root-only configuration to infer them.

## Boot and cache ordering

1. Cold boot clears BSS once per image and constructs that image's tables.
2. The primary publishes tables, executable code and boot stack storage to PoC
   before releasing any secondary. Cache-line sizes come from CTR_EL0, not a
   fixed maintenance stride.
3. Secondary assembly selects a private stack without using shared scratch
   storage. NS and Realm attach existing tables with a **stackless** MMU enable
   sequence before creating C frames. No frame spans uncached/cached execution.
   Local TLB and instruction-cache invalidation precede MMU enablement.
4. Each PE initializes its vectors, local execution state and fault-recovery
   context. Secondary entry never clears global BSS or rebuilds shared tables.
   Secure CPU_OFF framework handling invalidates that PE's initialization flag
   so a later warm boot restores its local registers.
5. Workers publish readiness only after joining coherent execution. CPU_ON
   success alone is not treated as readiness.

Live mailboxes use release/acquire operations. Do not add routine cache
invalidations around them: invalidating live dirty lines can lose updates.
PoE/PoPA maintenance for GPT/MEC scenarios is a separate requirement; the
mailbox protocol does not satisfy those rule-specific requirements.
Do not mutate shared mappings while workers can access them without a separate
break-before-make and system-wide TLB synchronization protocol. Existing MEC
alias manipulation remains a serial operation.

## Using workers

Include `val_host_mp.h`. Host startup calls `val_host_mp_init()` once.

1. `val_host_mp_start(pe)` checks that the target is offline, issues CPU_ON and
   waits for readiness.
2. `val_host_mp_dispatch(pe, callback, context)` publishes one job. The callback
   receives its PE index and context and returns `VAL_SUCCESS` or `VAL_ERROR`.
   Dispatch to several workers before waiting when actual overlap is required.
3. `val_host_mp_wait(pe, &result)` collects completion. A failed callback returns
   `VAL_ERROR`, not a misleading successful test result. The output is initialized
   to `VAL_ERROR` even for invalid calls or timeouts.
4. `val_host_mp_stop(pe)` accepts only an idle worker. It requests CPU_OFF and
   waits for firmware-confirmed OFF before allowing another boot.

Only the boot PE controls workers. A busy worker cannot be overwritten or
stopped. Callback code and contexts must remain valid throughout execution;
prefer persistent per-PE storage. Callbacks must restore local state and finish
their fault scopes before returning. Shared mutable state needs explicit
synchronization; ordinary `volatile` is not a substitute.

The five-second architected-counter waits use polling/yield, not an unbounded
WFE that needs an interrupt to make a deadline observable. A timeout or failed
boot quarantines that PE for the rest of the boot. A late completion cannot make
it reusable. After an unconfirmed completion, **do not free its context, restore
its test memory, or continue dependent tests**; reset the run. An SMC that never
returns still requires the external model/process timeout.

Do not dispatch arbitrary existing rule bodies concurrently. S-EL1 entry
scratch state, SVE/SME probe buffers, PCIe/IDE bookkeeping and caller-audit
scopes are still serial facilities. Input-capture auditing is owner-PE scoped
so it cannot suppress another PE's SMC, but starting multiple audit scopes is
not supported. Secure/Realm transport is checked serially per worker during
self-test; this is not parallel Secure/Realm rule-dispatch coverage.

## Validation

`FIRME_MP_SELF_TEST` defaults to `ON` in CMake and the FVP build wrapper.
Before rule execution it checks:

- All configured secondary PEs, including the other cluster.
- A rendezvous followed by concurrent cacheable-page exchange and per-PE
  expected-abort recovery, repeated for 16 rounds.
- Busy-job rejection, callback error propagation and subsequent successful work.
- A primary-PE caller-audit scope that must not intercept a secondary's SMC.
- Secure and Realm entry on every worker when that payload responds on the
  primary; unavailable worlds are explicitly shown in the self-test banner.
- Two CPU_ON/CPU_OFF cycles, followed by firmware-confirmed cleanup of workers.

Infrastructure failures stop the run before the rule summary; they are not
converted to rule PASS or SKIP. A single-PE PAL reports that no secondary
coverage was performed. The self-test is separate from rule/test counts.

Use the normal build/run instructions in the README. To exercise the model's
caches explicitly, set `FVP_CACHE_STATE_MODELLED=1` alongside the usual build and
model environment variables. `FIRME_MP_SELF_TEST=OFF` (wrapper) or
`-DFIRME_MP_SELF_TEST=OFF` (CMake) is a diagnostic escape hatch, not MP validation.

## Follow-up integration

Use this worker lifecycle for GPT continuation/observation scenarios only when
the required ABI and deterministic setup are available. Their registry gates
remain unchanged. Firmware-table ingestion, interrupt-driven preemption,
SMMU stream/DMA programming and parallelization of serial test scratch state
remain separate work; none is inferred from a successful worker self-test.
