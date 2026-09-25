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
| R0348 | `FIRME_GM_GPI_SET` | input | Gated | ns, secure, realm | Needs a state-aware transition adapter with deterministic setup and rollback. |
| R0349 | `FIRME_GM_GPI_SET` | output | Gated | ns, secure, realm | Needs controlled terminal and progress paths plus transition cleanup. |
| R0350 | `FIRME_GM_GPI_OP_CONTINUE` | input | Gated | ns, secure, realm | Needs a reproducible incomplete GM operation that returns a valid cookie. |
| R0351 | `FIRME_GM_GPI_OP_CONTINUE` | output | Gated | ns, secure, realm | Needs deterministic terminal, incomplete and busy continuation paths. |
| R0352 | `FIRME_GM_L1_GPT_CREATE` | input | Gated | realm | Needs platform protected-range discovery and L1 GPT object setup. |
| R0353 | `FIRME_GM_L1_GPT_CREATE` | output | Gated | realm | Needs create, continuation, negative-path and cleanup orchestration. |
| R0354 | `FIRME_GM_L1_GPT_DESTROY` | input | Gated | realm | Needs a successfully created L1 GPT object as deterministic pre-state. |
| R0355 | `FIRME_GM_L1_GPT_DESTROY` | output | Gated | realm | Needs create/destroy lifecycle setup, output correlation and cleanup. |
| R0356 | `FIRME_GM_GPT_INFO` | input | Gated | ns, secure, realm | Adapter complete; registry-gated until TF-A advertises mandatory GM_GPT_INFO. |
| R0357 | `FIRME_GM_GPT_INFO` | output | Gated | ns, secure, realm | Adapter complete; registry-gated until TF-A advertises mandatory GM_GPT_INFO. |
| R0358 | `FIRME_IDE_KEYSET_PROG` | input | Gated | ns, realm | Adapter implemented; gated until PAL supplies a real IDE path and deterministic status/effect evidence. |
| R0359 | `FIRME_IDE_KEYSET_PROG` | output | Gated | ns, realm | Adapter implemented; gated until PAL supplies Root Port evidence, progress control and rollback. |
| R0360 | `FIRME_IDE_KEYSET_GO` | input | Gated | ns, realm | Adapter implemented; gated until PAL supplies a real programmed Root Port/endpoint path and cleanup. |
| R0361 | `FIRME_IDE_KEYSET_GO` | output | Gated | ns, realm | Adapter implemented; gated until PAL supplies controlled Root Port state and rollback. |
| R0362 | `FIRME_IDE_KEYSET_STOP` | input | Gated | ns, realm | Adapter implemented; gated until PAL supplies a running IDE stream and safe cleanup. |
| R0363 | `FIRME_IDE_KEYSET_STOP` | output | Gated | ns, realm | Adapter implemented; gated until PAL supplies controlled running/stopped Root Port states. |
| R0364 | `FIRME_IDE_KEYSET_POLL` | input | Gated | ns, realm | Adapter implemented; gated until PAL supplies a pending IDE operation and real endpoint context. |
| R0365 | `FIRME_IDE_KEYSET_POLL` | output | Gated | ns, realm | Adapter implemented; gated until PAL controls pending, completed and aborted IDE operations. |
| R0366 | `FIRME_MEC_REFRESH` | input | Active | realm | Validate MEC_REFRESH reason and MECID input encoding. |
| R0367 | `FIRME_MEC_REFRESH` | output | Active | realm | Validate MEC_REFRESH terminal status output after BUSY retries. |

Every plan has a generated one-rule test entry point. Gated plans are
not added to the runtime registry until their adapters and platform
prerequisites support the stated rule coverage.
