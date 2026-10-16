# FIRME Alpha1 ABI Interface Coverage

Specification: `DEN0149 FIRME 1.0 ALP4`

This generated table is limited to the Alpha1 Base, GPT, IDE and MEC
ABIs. Attestation and Integrated Device Management are intentionally
outside this generator scope for this release.

| Rule | ABI | Direction | State | Worlds | Rationale |
|---|---|---|---|---|---|
| R0344 | `FIRME_SERVICE_VERSION` | input | Active | ns, secure, realm | Validate SERVICE_VERSION input register allocation and service ID selection. |
| R0345 | `FIRME_SERVICE_VERSION` | output | Active | ns, secure, realm | Validate SERVICE_VERSION version and NOT_SUPPORTED result encodings. |
| R0346 | `FIRME_SERVICE_FEATURES` | input | Active | ns, secure, realm | Validate SERVICE_FEATURES service and feature-register input selection. |
| R0347 | `FIRME_SERVICE_FEATURES` | output | Active | ns, secure, realm | Validate SERVICE_FEATURES status and feature-register result encodings. |
| R0348 | `FIRME_GM_GPI_SET` | input | Active | ns, secure, realm | Validate GM_GPI_SET base, count and target-GPI input allocation. |
| R0349 | `FIRME_GM_GPI_SET` | output | Active | ns, secure, realm | Validate GM_GPI_SET status, count and continuation-cookie outputs. |
| R0350 | `FIRME_GM_GPI_OP_CONTINUE` | input | Active | ns, secure, realm | Validate GM_GPI_OP_CONTINUE function ID and cookie input allocation. |
| R0351 | `FIRME_GM_GPI_OP_CONTINUE` | output | Active | ns, secure, realm | Validate GM_GPI_OP_CONTINUE status, count and cookie outputs. |
| R0352 | `FIRME_GM_L1_GPT_CREATE` | input | Active | realm | Validate GM_L1_GPT_CREATE protected-range and L1 object inputs. |
| R0353 | `FIRME_GM_L1_GPT_CREATE` | output | Active | realm | Validate GM_L1_GPT_CREATE status outputs across terminal and progress paths. |
| R0354 | `FIRME_GM_L1_GPT_DESTROY` | input | Active | realm | Validate GM_L1_GPT_DESTROY protected-range input allocation. |
| R0355 | `FIRME_GM_L1_GPT_DESTROY` | output | Active | realm | Validate GM_L1_GPT_DESTROY status and returned L1 object base. |
| R0356 | `FIRME_GM_GPT_INFO` | input | Active | ns, secure, realm | Validate GM_GPT_INFO inclusive base and exclusive top inputs. |
| R0357 | `FIRME_GM_GPT_INFO` | output | Active | ns, secure, realm | Validate GM_GPT_INFO status, returned top and protected-range state. |
| R0358 | `FIRME_IDE_KEYSET_PROG` | input | Active | ns, realm | Validate IDE_KEYSET_PROG ECAM, stream, key and handle inputs. |
| R0359 | `FIRME_IDE_KEYSET_PROG` | output | Active | ns, realm | Validate IDE_KEYSET_PROG terminal and asynchronous status outputs. |
| R0360 | `FIRME_IDE_KEYSET_GO` | input | Active | ns, realm | Validate IDE_KEYSET_GO ECAM, stream, keyset and handle inputs. |
| R0361 | `FIRME_IDE_KEYSET_GO` | output | Active | ns, realm | Validate IDE_KEYSET_GO terminal and asynchronous status outputs. |
| R0362 | `FIRME_IDE_KEYSET_STOP` | input | Active | ns, realm | Validate IDE_KEYSET_STOP ECAM, stream, keyset and handle inputs. |
| R0363 | `FIRME_IDE_KEYSET_STOP` | output | Active | ns, realm | Validate IDE_KEYSET_STOP terminal and asynchronous status outputs. |
| R0364 | `FIRME_IDE_KEYSET_POLL` | input | Active | ns, realm | Validate IDE_KEYSET_POLL ECAM, response selector and keyset inputs. |
| R0365 | `FIRME_IDE_KEYSET_POLL` | output | Active | ns, realm | Validate IDE_KEYSET_POLL status, handle and keyset outputs. |
| R0366 | `FIRME_MEC_REFRESH` | input | Active | realm | Validate MEC_REFRESH reason and MECID input encoding. |
| R0367 | `FIRME_MEC_REFRESH` | output | Active | realm | Validate MEC_REFRESH terminal status output after BUSY retries. |

Every active plan is registered and executes its applicable cases. Missing
owned memory, a retained operation or an observable endpoint path produces
SKIP, not PASS. The runner still executes later cases and preserves any
failure. Missing mandatory ABIs and incorrect DUT results produce FAIL.
A registered rule is not a claim that every listed status was observed.
