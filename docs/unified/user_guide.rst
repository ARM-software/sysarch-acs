Unified Command Line Options
============================

Usage::

  Unified.efi [options]

Options
-------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``-a`` ``{bsa|sbsa|pcbsa}``
     - Architecture selection. ``bsa`` uses the full BSA rule checklist, ``sbsa`` uses the full SBSA rule checklist, and ``pcbsa`` uses the full PC BSA rule checklist.
   * - ``-cache``
     - Indicate that the test system supports PCIe address translation cache.
   * - ``-dtb``
     - Dump the Device Tree Blob (DTB) file.
   * - ``-el1physkip``
     - Skip EL1 register checks. VE systems run ACS at EL1 and some systems crash when accessing EL1 registers; use for debugging only.
   * - ``-f`` *<path>*
     - Specify the log file that records the test results.
   * - ``-fr``
     - Run rules up to the Future Requirements (FR) level, validated against the ``-a`` selection (for example, ``-a sbsa -fr``).
   * - ``-h``, ``-help``
     - Print the usage message.
   * - ``-l`` *<level>*
     - Run compliance tests up to the provided level, validated against the ``-a`` selection (for example, ``-a sbsa -l 7``). Example: ``-l 4``.
   * - ``-m`` *<modules>*
     - Run only the specified modules (comma-separated names). Accepted values: ``PE``, ``GIC``, ``PERIPHERAL``, ``MEM_MAP``, ``PMU``, ``RAS``, ``SMMU``, ``TIMER``, ``WATCHDOG``, ``NIST``, ``PCIE``, ``MPAM``, ``ETE``, ``TPM``, ``POWER_WAKEUP``. Example: ``-m PE,GIC,PCIE``.
   * - ``-mmio``
     - Enable ``pal_mmio_read``/``pal_mmio_write`` prints; use with ``-v 1``.
   * - ``-no_crypto_ext``
     - Declare that the cryptography extension is not supported due to export restrictions.
   * - ``-only`` *<level>*
     - Run tests only for rules at the provided level, validated against the ``-a`` selection (for example, ``-a sbsa -only 4``).
   * - ``-os``, ``-hyp``, ``-ps``
     - Software view filters that work with ``-a bsa`` only; flags can be combined. ``-os`` runs BSA operating system software view tests, ``-hyp`` runs BSA hypervisor software view tests, and ``-ps`` runs BSA platform security software view tests.
   * - ``-p2p``
     - Indicate that the PCIe hierarchy supports peer-to-peer.
   * - ``-r`` *<rules>*
     - Run tests for the provided comma-separated rule IDs or a rules file (for example, ``-r B_PE_01,B_PE_02,B_GIC_01`` or ``-r rules.txt``). Files may mix commas and newlines; lines starting with ``#`` are treated as comments.
   * - ``-slc`` *<type>*
     - Provide the system last-level cache type: ``1`` for PPTT PE-side cache, ``2`` for HMAT memory-side cache.
   * - ``-skip`` *<rules>*
     - Skip the specified rule IDs (comma-separated, similar to ``-r``). Example: ``-skip B_PE_01,B_GIC_02``.
   * - ``-skip-dp-nic-ms``
     - Skip PCIe tests for DisplayPort, Network, and Mass Storage devices.
   * - ``-skipmodule`` *<modules>*
     - Skip the specified modules (comma-separated names). Example: ``-skipmodule PE,GIC,PCIE``.
   * - ``-timeout`` *<value>*
     - Set the timeout multiplier for wakeup tests (minimum 1, maximum 5; defaults to 1).
   * - ``-v`` *<level>*
     - Set the verbosity of the prints (``1`` prints all messages, ``5`` prints only errors).
