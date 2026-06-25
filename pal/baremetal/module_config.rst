Module & MMU Configuration User Guide
=====================================

Overview
--------

The BSA ACS bare-metal framework allows flexible control over:

* **Which modules are built by default** (compile-time control)
* **Which modules run by default** (build-time list)
* **Which modules run for a specific boot** (run-time override via EL3)
* **Whether MMU setup is enabled** (compile-time flag)

This document explains how to use these configuration options in a clear,
simple way.

-------------------------------------
Verbosity Control (ACS_VERBOSE_LEVEL)
-------------------------------------
You can provide Verbosity to the build system to specify the
print level of the ACS

**Default:** Print Level is ACS_PRINT_TEST (3)
**Change Print Level** define `ACS_VERBOSE_LEVEL=<PRINT_LVL>` at CMake configure time.

`PRINT_LVL` can range from  ACS_PRINT_INFO (1) to ACS_PRINT_ERR (5)
- ACS_PRINT_INFO  (1)
- ACS_PRINT_DEBUG (2)
- ACS_PRINT_TEST  (3)
- ACS_PRINT_WARN  (4)
- ACS_PRINT_ERR   (5)

Both macros and numerics can be used

Examples
~~~~~~~~
.. code-block:: bash

   cmake .. -DACS_VERBOSE_LEVEL=ACS_PRINT_INFO
                  or
   cmake .. -DACS_VERBOSE_LEVEL=1

-------------------------------------
Compliance Level Control (ACS_LEVEL)
-------------------------------------
You can override the target compliance level of the active ACS suite at
CMake configure time. When ``ACS_LEVEL`` is defined it takes precedence
over the per-platform ``PLATFORM_OVERRIDE_<ACS>_LEVEL`` value supplied by
``pal/baremetal/target/<TARGET>/include/platform_override_fvp.h``.

**Default:** value of ``PLATFORM_OVERRIDE_<ACS>_LEVEL`` from the platform
header, with ``LVL_FILTER_MAX`` filter mode.

**Set Level:** define ``ACS_LEVEL=<value>`` at CMake configure time.

Accepted values:

- A positive integer ``<n>`` — sets ``ctx->level_value = <n>`` and
  selects ``LVL_FILTER_MAX`` (run all rules with level ``<=`` n).
- ``fr`` (or ``FR``) — selects ``LVL_FILTER_FR`` (future-requirements
  mode). ``ctx->level_value`` is left at the platform default.

Per-ACS valid numeric ranges:

- ``-DACS=bsa``    : ``1``
- ``-DACS=sbsa``   : ``3..7``
- ``-DACS=pc_bsa`` : ``1``

Numeric values outside the supported range are clamped to the nearest
valid level at runtime, with a console warning.

Examples
~~~~~~~~
.. code-block:: bash

   cmake --preset sbsa -DACS_LEVEL=7
   cmake --preset sbsa -DACS_LEVEL=fr
   cmake --preset bsa  -DACS_LEVEL=1

-------------------------------------
MMU Control (ACS_ENABLE_MMU)
-------------------------------------

The ACS enables the MMU during startup unless you explicitly disable it.

**Default:** MMU enabled
**Disable MMU:** define `ACS_ENABLE_MMU=0` at CMake configure time.

Examples
~~~~~~~~

Enable MMU (default):

.. code-block:: bash

   cmake .. -DTARGET=RDV3CFG1
   make bsa

Disable MMU (recommended for simulators):

.. code-block:: bash

   cmake .. -DTARGET=RDV3CFG1 -DACS_ENABLE_MMU=0
   make bsa

-------------------------------------
Build-Time Module Selection
-------------------------------------

You can provide a list of **module base IDs** to the build system to specify
which ACS modules should be enabled *by default* in the firmware image.

The list must contain **ACS module base identifiers**, such as:

- `ACS_PE_TEST_NUM_BASE`
- `ACS_TIMER_TEST_NUM_BASE`
- `ACS_PCIE_TEST_NUM_BASE`
- `ACS_EXERCISER_TEST_NUM_BASE`
- (and others defined in ACS)

If no list is provided, **all modules are enabled** by default.

Providing a Build-Time List
~~~~~~~~~~~~~~~~~~~~~~~~~~~

To choose specific modules at build time, use:

.. code-block:: bash

   cmake .. \
     -DTARGET=RDV3CFG1 \
     -DACS_ENABLED_MODULE_LIST="ACS_PE_TEST_NUM_BASE,ACS_PCIE_TEST_NUM_BASE"
   make bsa

This produces an ACS image where:

* Only **PE** and **PCIe** modules run by default
* All other modules are skipped
* Unless overridden at runtime (see next section)

-------------------------------------
Run-Time Module Override (EL3)
-------------------------------------

If EL3 firmware provides a module list at boot time, that list overrides:

1. The build-time list (`ACS_ENABLED_MODULE_LIST`)
2. The default “run everything” behavior

This enables platforms to choose a different module set at boot **without
rebuilding ACS**.

The EL3-provided list uses the same module base IDs as the build-time list.

Example Use-Cases
~~~~~~~~~~~~~~~~~

* Boot with **only PCIe tests** on pre-silicon bring-up
* Skip **SMMU** tests for early integration
* Enable **Exerciser** tests only when hardware is present

As long as EL3 provides a valid override list, ACS will execute *only* those
modules.

-------------------------------------
Module Selection Priority
-------------------------------------

ACS determines which modules to execute using this precedence order:

1. **Run-time override** (EL3-provided list)
2. **Build-time list** (`ACS_ENABLED_MODULE_LIST`)
3. **Default:** all modules enabled

This ensures:

* EL3 has full control when needed
* Build-time defaults apply otherwise
* Legacy behavior remains unchanged if no configuration is provided

-------------------------------------
Examples
-------------------------------------

Build ACS with default settings:

.. code-block:: bash

   cmake .. -DTARGET=RDV3CFG1
   make bsa

Build ACS enabling only PE + Timer modules:

.. code-block:: bash

   cmake .. \
     -DTARGET=RDV3CFG1 \
     -DACS_ENABLED_MODULE_LIST="PE,TIMER"
   make bsa

Run ACS with modules overridden by EL3 (example platform firmware):

- EL3 prepares its own module list
- ACS executes only those modules
- Build-time settings are ignored

## 📄 License

Distributed under [Apache v2.0 License](https://www.apache.org/licenses/LICENSE-2.0).
© 2026 Arm Limited and Contributors.
