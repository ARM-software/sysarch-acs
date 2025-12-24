# Release readiness checklist (Baremetal)

Use this checklist before you upstream/check-in a new platform port.

## Documentation
- [ ] `docs/baremetal/README.md` links are valid
- [ ] Platform-specific bring-up notes are captured somewhere discoverable
- [ ] Override guides used are referenced/linked

## Build reproducibility
- [ ] Clean build works from scratch (documented steps)
- [ ] Toolchain requirements documented
- [ ] Platform selection mechanism documented

## Functional coverage
- [ ] Smoke run passes (define what smoke is for your suite set)
- [ ] No hard faults / aborts in normal run
- [ ] Logs contain platform ID and build ID for traceability

## Platform override correctness
- [ ] Counts match number of populated entries for each block
- [ ] Memory map has no overlaps
- [ ] ECAM and MMIO windows validated
- [ ] IORT ID mappings validated (unit test or sanity dump)

## Artifact hygiene
- [ ] Example logs attached to PR (or referenced)
- [ ] Known limitations documented (what doesn’t work yet)
