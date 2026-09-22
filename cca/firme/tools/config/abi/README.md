# FIRME ABI test generation

`firme_1_0_alp4.json` is the reviewed ABI catalog. The `release_abis` array is
the Alpha1 allowlist. Per-ABI scenarios live in `scenarios/alpha1/` and define
the input and output rule checks, mutations, expected results, caller worlds,
and registry state.

Generate the ABI catalog and test-plan declarations and definitions with:

```sh
python3 cca/firme/tools/scripts/generate_firme_abi_catalog.py
```

Verify checked-in generated files without modifying them with:

```sh
python3 cca/firme/tools/scripts/generate_firme_abi_catalog.py --check
```

This framework generates only `val_firme_abi_catalog.h` and
`val_firme_abi_catalog.c`. It does not generate executable wrappers or registry
entries. Those require service-specific adapters to prepare valid state,
validate outputs and effects, and clean up, and are introduced with the
generated ABI tests.

Disabled plans must provide a non-empty `disabled_reason`. All plans are
schema-validated; a plan declaration alone does not register a runnable test.

Run the generator infrastructure regression checks with:

```sh
python3 cca/firme/tools/scripts/test_generate_firme_abi_catalog.py
```
