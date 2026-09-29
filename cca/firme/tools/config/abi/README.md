# FIRME ABI test generation

`firme_1_0_alp4.json` is the reviewed ABI catalog. The `release_abis` array is
the Alpha1 allowlist. Per-ABI scenarios live in `scenarios/alpha1/` and define
the input and output rule checks, mutations, expected results, caller worlds,
and registry state.

Generate the catalog, plans, one-rule test sources, and registry fragments
with:

```sh
python3 cca/firme/tools/scripts/generate_firme_abi_catalog.py
```

Verify checked-in generated files without modifying them with:

```sh
python3 cca/firme/tools/scripts/generate_firme_abi_catalog.py --check
```

Each enabled ABI has a handwritten adapter. The adapter supplies valid state
and inputs, applies generated mutations, validates ABI-specific outputs or
effects, and cleans up. The generator supplies the case loop, result
aggregation, per-world wrappers, and suite registry entries. It emits one
source per interface rule under `test/<module>/generated_abi_*.c`.

Disabled plans must provide a non-empty `disabled_reason`. They remain in the
generated Alpha1 coverage report and have a compiled one-rule entry point,
but no registry entry is emitted until the required adapter is complete.

Run the generator infrastructure regression checks with:

```sh
python3 cca/firme/tools/scripts/test_generate_firme_abi_catalog.py
```

The catalog also records the eight-bit FIRME service-ID width and every ALP4
feature-register size. Generator validation checks these ACS declarations; it
does not establish DUT compliance with R0023 or R0041. Both rules are proposed
Alpha1 DUT-validation exclusions pending Architect approval. Non-Base feature
register 1 descriptors include the 16-bit `MAX_CONCURRENT_OP_CNT` field used
by the strict R0064 and R0065 runtime bodies.
