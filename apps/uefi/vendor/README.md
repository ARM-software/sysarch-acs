# Vendor INF Files

Vendor-specific INF files that extend public sysarch-acs with vendor tests.

## Architecture

```
apps/uefi/
├── Bsa.inf                      # Public BSA
├── Sbsa.inf                     # Public SBSA
├── xbsa_acpi.inf                # Public xBSA ACPI
└── vendor/
    ├── verify_vendor_inf.sh     # Sync verification script
    └── nvidia/
        ├── BsaNvidia.inf        # BSA + NVIDIA tests
        ├── SbsaNvidia.inf       # SBSA + NVIDIA tests
        └── XbsaAcpiNvidia.inf   # xBSA ACPI + NVIDIA tests
```

## Build Commands

```bash
# Public builds (no vendor argument)
source acsbuild.sh bsa
source acsbuild.sh sbsa
source acsbuild.sh xbsa_acpi

# Vendor builds (add vendor name as second argument)
source acsbuild.sh bsa nvidia
source acsbuild.sh sbsa nvidia
source acsbuild.sh xbsa_acpi nvidia
```

## Build-Time Sync Check

Vendor builds automatically verify that vendor INFs include all upstream tests.
If sync is broken, the build fails and shows missing tests.

Manual verification:
```bash
./apps/uefi/vendor/verify_vendor_inf.sh nvidia
```

## Adding a New Vendor

1. Create directory: `apps/uefi/vendor/<vendor>/`

2. Create vendor INFs (copy from nvidia/ and modify):
   - Change `BASE_NAME` and `FILE_GUID`
   - Capitalize vendor name in INF filename (e.g., `BsaQualcomm.inf`)
   - Update paths and vendor test sources

3. The build script auto-detects vendor INFs by naming convention:
   - `<AcsType><Vendor>.inf` (e.g., `BsaNvidia.inf`, `XbsaAcpiQualcomm.inf`)

## Syncing with Upstream

When upstream adds/removes tests:
1. Run `./verify_vendor_inf.sh <vendor>` to find missing tests
2. Update vendor INFs accordingly
3. Re-verify before building
