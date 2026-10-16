# Native IDE regressions

Run `bash tools/scripts/check_ide_native.sh` from the FIRME directory.
Set `CC=clang` to select Clang or
`SANITIZE=1` to enable AddressSanitizer and UndefinedBehaviorSanitizer.
The runner creates and removes a temporary FIRME build directory. It never
invokes firmware, an FVP, a cross-build, or a real PCI configuration access.

- `pcie_bounds_test.c` rejects overflowing identifiers, invalid device/function
  fields, and unaligned/out-of-function configuration accesses before PAL calls.
- `ide_completion_test.c` injects wrong POLL handles, wrong poll-any keysets,
  nonterminal/error programming results, poisoned unused registers, synchronous
  POLL setup, and a generated-ABI completion on the final permitted retry. It also
  verifies that link-state keys/handles remain owned after a bad completion echo
  or poll timeout, and that such state cannot be overwritten or released.
  CXL field rules reject wrong PROG, GO, and STOP completion handles while
  accepting synchronous results and ignoring unused exact-mode POLL `x2`.
- `spdm_buffer_test.c` checks the actual requester callbacks against libspdm's
  full-allocation buffer contract, including capacity canaries and release rules.
  Host namespace checks reject identical keys/IDs, unavailable endpoint directions,
  STOP/GO and observation failures; failed cleanup retains ownership. Both NS and
  Realm routing are exercised with mocked Root Port commands.
- `check_ide_namespace_native.sh` checks the directional CXL traffic observer:
  RX STOP must leave authenticated TX writes working, reject read responses,
  and preserve the written data after RX restarts. Fault injections reject
  missing counters, unrelated faults, changed ownership and dropped writes.

SPDM tests require the existing pinned libspdm and spdm-emu headers. The fixture
trust-anchor header is intentionally not a usable certificate: no negotiation or
authentication is exercised. Linker garbage collection removes unrelated runtime
paths; these tests do not establish end-to-end SPDM or IDE traffic coverage.
Only shift sanitization is disabled for the SPDM translation unit because the
pinned libspdm status macros use signed `8 << 28`; other UBSan checks and ASan
remain enabled. No third-party sources are changed.
