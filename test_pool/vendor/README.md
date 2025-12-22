# Vendor Test Pool

This directory contains vendor-specific tests that extend the standard BSA/SBSA test suites.

## Directory Structure

```
vendor/
├── README.md           # This file
├── nvidia/             # NVIDIA-specific tests
│   ├── README.md       # NVIDIA vendor test documentation
│   └── timer/          # Timer-related vendor tests
│       ├── vt001.c     # Vendor timer test 001
│       └── ...
├── <vendor>/           # Other vendor directories
│   └── ...
└── template/           # Template for new vendor tests
    └── vtest_template.c
```

## Adding Vendor Tests

### 1. Create Vendor Directory

Create a new directory under `vendor/` with your vendor name (lowercase):

```bash
mkdir -p test_pool/vendor/<vendor_name>/timer
```

### 2. Test Numbering Convention

Vendor tests use a separate numbering space to avoid conflicts with standard tests:

```c
/* Standard test numbering (reserved 0-9999) */
#define ACS_TIMER_TEST_NUM_BASE      400

/* Vendor test numbering (10000+) */
#define ACS_VENDOR_TEST_NUM_BASE     10000

/* Per-vendor offsets (1000 tests per vendor) */
#define ACS_VENDOR_NVIDIA_BASE       (ACS_VENDOR_TEST_NUM_BASE + 0)
#define ACS_VENDOR_QUALCOMM_BASE     (ACS_VENDOR_TEST_NUM_BASE + 1000)
#define ACS_VENDOR_MEDIATEK_BASE     (ACS_VENDOR_TEST_NUM_BASE + 2000)
/* Add more vendors as needed */

/* Module offsets within vendor space (100 tests per module) */
#define VENDOR_TIMER_OFFSET          0
#define VENDOR_PCIE_OFFSET           100
#define VENDOR_PE_OFFSET             200
/* ... */
```

### 3. Test File Naming

Vendor tests should be named with a `v` prefix:
- `vt001.c` - Vendor timer test 001
- `vp001.c` - Vendor PCIe test 001
- `vpe001.c` - Vendor PE test 001

### 4. Test Rule ID Convention

Use vendor-prefixed rule IDs:
```c
#define TEST_RULE  "NVIDIA_TIME_01"  /* Vendor-specific rule */
```

### 5. Creating a Test

Copy the template and modify:

```c
#include "val/include/acs_val.h"
#include "val/include/acs_timer.h"
#include "val/include/acs_pe.h"

/* Test identification */
#define TEST_NUM   (ACS_VENDOR_NVIDIA_BASE + VENDOR_TIMER_OFFSET + 1)
#define TEST_DESC  "NVIDIA: Custom timer validation"
#define TEST_RULE  "NVIDIA_TIME_01"

static void payload(uint32_t num_pe)
{
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    
    /* Your test logic here */
    
    val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
vt001_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    status = val_initialize_test(TEST_NUM, TEST_DESC, val_pe_get_num());
    if (status != ACS_STATUS_SKIP)
        payload(num_pe);

    status = val_check_for_error(TEST_NUM, 1, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);
    
    return status;
}
```

### 6. Registering Vendor Tests

Add your test entry point to the appropriate execute file or create a vendor-specific executor.

## Running Vendor Tests

### Run All Vendor Tests
```bash
xbsa_acpi.efi -m VENDOR -v 1
```

### Run Specific Vendor Tests
```bash
xbsa_acpi.efi -r NVIDIA_TIME_01 -v 1
```

### Run Standard + Vendor Tests
```bash
xbsa_acpi.efi -m BSA,VENDOR -v 1
```

## Build Architecture

```
apps/uefi/
├── xbsa_acpi.inf                  # Public INF (unchanged from upstream)
└── vendor/nvidia/
    └── XbsaAcpiNvidia.inf         # Public tests + NVIDIA vendor tests
```

Vendor INF files copy the public test list and add vendor tests at the end.
When upstream is updated, sync the test list in vendor INF files.

### DSC Integration

```ini
[Components]
  # Public build
  ShellPkg/Application/sysarch-acs/apps/uefi/xbsa_acpi.inf

  # OR NVIDIA internal build
  ShellPkg/Application/sysarch-acs/apps/uefi/vendor/nvidia/XbsaAcpiNvidia.inf
```

## Guidelines

1. **Isolation**: Vendor tests should not modify or depend on standard test behavior
2. **Documentation**: Each vendor directory must have a README.md
3. **Compatibility**: Use only public VAL/PAL APIs
4. **Naming**: Follow the naming conventions strictly
5. **Testing**: Ensure vendor tests pass on the target platform before submission

## Contact

For questions about adding vendor tests, contact the sysarch-acs maintainers.

