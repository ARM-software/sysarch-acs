# Porting checklist (Baremetal)

Use this checklist while bringing up a new platform.

## Boot & console
- [ ] Can boot a minimal payload
- [ ] UART console prints reliably
- [ ] UART config matches actual IP and base
- [ ] UART region is marked DEVICE in memory map

## Memory map
- [ ] DRAM regions are correct and marked NORMAL
- [ ] MMIO windows are marked DEVICE
- [ ] Reserved carveouts marked RESERVED
- [ ] Holes/unbacked regions marked NOT_POPULATED (if required)

## Timers & watchdog
- [ ] Per-CPU timer GSIVs correct (PPIs)
- [ ] Timer flags (mode/polarity/always-on) correct
- [ ] CNTFRQ correct
- [ ] Platform timer frames correct (if used)
- [ ] Watchdog bases + GSIVs correct (if used)

## PCIe (if used)
- [ ] ECAM base + bus ranges correct
- [ ] Enumeration limits correct
- [ ] PCIe MMIO resource windows don’t overlap
- [ ] Static device list (if used) matches enumeration

## IORT / SMMU (if used)
- [ ] SMMU base addresses correct
- [ ] ITS count/grouping consistent with firmware
- [ ] RID→StreamID mappings correct
- [ ] StreamID→DeviceID mappings correct
- [ ] No overlapping StreamID windows

## Optional blocks (as required by suites)
- [ ] Cache/topology overrides
- [ ] HMAT bandwidth hints
- [ ] PMU nodes
- [ ] RAS / RAS2 blocks

## Repro artifacts
- [ ] Save binary + git SHA
- [ ] Save override header
- [ ] Save UART log
