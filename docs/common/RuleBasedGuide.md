# Rule-based execution in sysarch-acs

The **sysarch-acs** has undergone framework changes to support **rule-based execution**.
This brings ACS execution and reporting **closer to the specification**.

**Migration status**

* **Moved to new design:** BSA, SBSA, PC-BSA
* **WIP:** Sys MPAM, DRTM, PFDI

---

## How to Run (rule-based)

```
Bsa.efi/Sbsa.efi/PCBsa.efi -r "<rule id>"` **or** `Bsa.efi/Sbsa.efi/PCBsa.efi -r "<rule id collection>"
```

**Examples**

* To run test for a single rule:
  ```bash
  Bsa.efi -r B_PE_04
  ```

* To run test for a multiple rules:
  ```bash
  Bsa.efi -r B_PE_04,B_SMMU_01
  ```

* To run for full complaince, no parameter needs to be passed:
  ```bash
  Bsa.efi
  ```

* To run by passing a text file which contains rule list:
  - The rule must be comma seperated

  ```bash
  # rule.txt which contains B_PE_01,B_PE_02,B_GIC_01

  Bsa.efi -r rule.txt
  ```

---

## Log Format

```
START <MODULE> <RULEID> <TESTNUM> <TEST_DESC>
... debug/info prints ...                      # Based on verbosity set
END <RULEID> <STATUS>
```

**Examples**

* Log format when rule has no sub-rules and covered by single test
```
START PE B_PE_01 1 : Check Arch symmetry across PE
    test_pool/pe/pe001.c:411 pe001_entry
       Primary PE Index    : 0
       Primary PE MIDR_EL1 : 0x410FD840
       Other Cores         : Identical       
END B_PE_01 PASSED
```

* Log format when rule has no sub-rules and is covered by multiple test
```
START GIC B_PPI_00 - : PPI Assignments check
    test_pool/gic/g006.c:84 g006_entry
    test_pool/gic/g007.c:88 g007_entry
    test_pool/gic/g009.c:115 g009_entry
        Failed at checkpoint -  2
    test_pool/gic/g010.c:101 g010_entry
    test_pool/gic/g011.c:114 g011_entry
END B_PPI_00 FAILED
```

* Log format when rule has sub-rules
```
START SMMU B_SMMU_21 - : SMMUv3 Integration compliance

    START SMMU SMMU_01 307 : SMMUv3 Integration compliance
    test_pool/smmu/i007.c:80 i007_entry
    END SMMU_01 PASSED

    START - SMMU_02 - : 
    END SMMU_02 NOT TESTED (TEST NOT IMPLEMENTED)
END B_SMMU_21 PASSED(*PARTIAL)
```


* Log format when rule is not covered by ACS
```
START - B_SMMU_12 - : 
END B_SMMU_12 NOT TESTED (TEST NOT IMPLEMENTED)
```
`NOTE: With rule-based design, ACS also capture list of rules which are not implemented and needs to verified by alternative approach`

---

## Debug using logs

```
START GIC B_PPI_00 - : PPI Assignments check
    test_pool/gic/g006.c:84 g006_entry
    test_pool/gic/g007.c:88 g007_entry
    test_pool/gic/g009.c:115 g009_entry
        Failed at checkpoint -  2
    test_pool/gic/g010.c:101 g010_entry
    test_pool/gic/g011.c:114 g011_entry
END B_PPI_00 FAILED
```

The test code can be reached using the file name and test entry function
- `test_pool/gic/g009.c      # is test file relative path from test_pool directory`
- `g009_entry                # is test entry function in the test file`
- `Failed at checkpoint - 2  # In test the **val_set_status** which is setting **RESULT_FAIL(TEST_NUM, 2)** is test failure case`
