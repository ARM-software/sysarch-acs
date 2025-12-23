EL3-Driven Module Selection
===========================

Overview
--------

In addition to build-time and runtime module selection, ACS supports
a simple mechanism for **EL3 firmware** to tell ACS which modules
and/or tests to run.

This is useful when:

* EL3 wants to enforce a policy (for example, only PCIe tests).
* You want to avoid command-line/runtime configuration in NS-EL2.
* You need per-boot, firmware-controlled selection.

The mechanism is:

* EL3 passes a pointer to a parameter structure in a shared memory region.
* EL3 also passes a magic value to indicate that parameters are valid.
* ACS reads the structure once at boot and overrides its internal tables.

Register Convention
-------------------

When jumping from EL3 to the ACS entry point (``acs_entry``):

* **X19** must contain the magic value ``ACS_EL3_PARAM_MAGIC``.
* **X20** must contain the address of a ``bsa_el3_params`` structure.

If X19 does **not** match ``ACS_EL3_PARAM_MAGIC``, ACS will **ignore X20**
and run with its normal configuration (command-line / default modules).

Parameter Structure
-------------------

The structure layout is defined in ``acs_el3_param.h`` as:

.. code-block:: c

   #define ACS_EL3_PARAM_MAGIC  0x425341454C335031ULL  /* 'BSAEL3P1' */

   typedef struct {
     uint64_t version;              /* 0 or 1 */

     /* Optional: override test list */
     uint64_t test_array_addr;      /* uint32_t[] of test IDs (can be 0) */
     uint64_t test_array_count;     /* number of entries in test_array_addr */

     /* Optional: override module list */
     uint64_t module_array_addr;    /* uint32_t[] of module IDs (can be 0) */
     uint64_t module_array_count;   /* number of entries in module_array_addr */

     /* Reserved for future use */
     uint64_t reserved0;
     uint64_t reserved1;
   } bsa_el3_params;

Notes:

* The structure must be in memory that NS-EL2 / ACS can read.
* Only the **address** of the structure is passed in X20.
* ``version`` is used by ACS to check compatibility (currently 0 or 1).
* Either tests, modules, both, or neither may be provided.

Module and Test IDs
-------------------

Module IDs are the **existing ACS test number bases**. For example:

.. code-block:: c

   #define ACS_PE_TEST_NUM_BASE         0
   #define ACS_MEMORY_MAP_TEST_NUM_BASE 100
   #define ACS_GIC_TEST_NUM_BASE        200
   #define ACS_SMMU_TEST_NUM_BASE       300
   #define ACS_TIMER_TEST_NUM_BASE      400
   #define ACS_WAKEUP_TEST_NUM_BASE     500
   #define ACS_PER_TEST_NUM_BASE        600
   #define ACS_WD_TEST_NUM_BASE         700
   #define ACS_PCIE_TEST_NUM_BASE       800
   #define ACS_PCIE_EXT_TEST_NUM_BASE   900
   #define ACS_MPAM_TEST_NUM_BASE       1000
   #define ACS_PMU_TEST_NUM_BASE        1100
   #define ACS_RAS_TEST_NUM_BASE        1200
   #define ACS_NIST_TEST_NUM_BASE       1300
   #define ACS_ETE_TEST_NUM_BASE        1400
   #define ACS_EXERCISER_TEST_NUM_BASE  1500
   #define ACS_TPM2_TEST_NUM_BASE       1600

You can pass either:

* the **base ID** (for example ``ACS_PCIE_TEST_NUM_BASE``), or
* any test number inside that module’s range.

ACS will map each ID to an internal module bitmask and then derive the
runtime mask (``g_enabled_modules``) as usual.

What ACS Does Internally
------------------------

At boot, ACS:

1. Saves X19 and X20 into globals (``g_el3_param_magic``, ``g_el3_param_addr``)
   in ``BsaBootEntry.S``.
2. In ``ShellAppMainbsa()``, calls a helper (``bsa_apply_el3_params()``) which:
   * Checks that ``g_el3_param_magic == ACS_EL3_PARAM_MAGIC``.
   * Checks that the parameter structure address is non-zero.
   * Checks that ``version`` is acceptable.
   * If valid:
     - Overrides ``g_execute_tests`` / ``g_num_tests`` if test override is given.
     - Overrides ``g_execute_modules`` / ``g_num_modules`` if module override is given.
3. Uses the (possibly overridden) ``g_execute_modules`` / ``g_num_modules`` to
   compute ``g_enabled_modules``, and then:
   * Creates only the required information tables.
   * Executes only the enabled modules.

If X19 does not contain ``ACS_EL3_PARAM_MAGIC`` or the structure is invalid,
ACS falls back to its usual behavior (no EL3 override).

Simple EL3 Example
------------------

Below is a minimal example showing how EL3 can force ACS to run only
PCIe and Exerciser modules.

.. code-block:: c

   #include "acs_el3_param.h"

   static uint32_t modules_to_run[] = {
       ACS_PCIE_TEST_NUM_BASE,
       ACS_EXERCISER_TEST_NUM_BASE,
   };

   static bsa_el3_params acs_params __attribute__((aligned(64))) = {
       .version            = 1,
       .test_array_addr    = 0,     /* no test overrides */
       .test_array_count   = 0,
       .module_array_addr  = (uint64_t)modules_to_run,
       .module_array_count = sizeof(modules_to_run) / sizeof(uint32_t),
       .reserved0          = 0,
       .reserved1          = 0,
   };

   void jump_to_acs(uint64_t acs_entry_pa)
   {
       register uint64_t x19 __asm__("x19") = ACS_EL3_PARAM_MAGIC;
       register uint64_t x20 __asm__("x20") = (uint64_t)&acs_params;

       /* Program the NS-EL2 entry point and perform the world switch here.
        * The exact code depends on your firmware (TF-A, custom EL3, etc.).
        *
        * After the switch, execution will start at acs_entry_pa
        * (acs_entry) with:
        *   x19 = ACS_EL3_PARAM_MAGIC
        *   x20 = &acs_params
        */
   }

If you want ACS to **ignore** EL3 parameters and run normally,
do **not** set X19 to the magic value (for example, leave X19 = 0).

Requirements for Partners
-------------------------

* ``bsa_el3_params`` must live in memory that NS-EL2 can read.
* X19 must be set to ``ACS_EL3_PARAM_MAGIC`` to enable the override.
* X20 must hold the address of a valid ``bsa_el3_params`` structure.
* The module IDs in ``module_array_addr`` must be valid ACS test IDs
  (typically the ``ACS_*_TEST_NUM_BASE`` values).
* If ``module_array_count == 0``, ACS will not override modules and will
  behave as if no module list was provided.

This interface allows platform firmware to control ACS execution in a
simple, well-defined way without changing the ACS build or command-line
configuration.


## 📄 License

Distributed under [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).
© 2026 Arm Limited and Contributors.

