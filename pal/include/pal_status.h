 /** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/


#ifndef PAL_STATUS_H_
#define PAL_STATUS_H_

#include "acs_stdint.h"

typedef enum pal_status {
  PAL_STATUS_SUCCESS                = 0x00000000u,
                /* Operation completed; outputs valid. */
  PAL_STATUS_FEATURE_SUPPORTED      = 0x00000001u,
                /* Capability present / positive probe result. */
  PAL_STATUS_FEATURE_UNSUPPORTED    = 0x00000002u,
                /* Capability absent / negative probe result. */
  PAL_STATUS_ERROR                  = 0x80000000u,
                /* Generic failure when no better code fits. */
  PAL_STATUS_INVALID_PARAM          = 0x80000001u,
                /* Bad arguments from caller (NULL/out of range). */
  PAL_STATUS_UNSUPPORTED            = 0x80000002u,
                /* Platform lacks the capability; no implementation planned. */
  PAL_STATUS_NO_RESOURCE            = 0x80000003u,
                /* Required buffer/register/resource unavailable. */
  PAL_STATUS_BUSY                   = 0x80000004u,
                /* Hardware or service busy; retry later. */
  PAL_STATUS_TIMEOUT                = 0x80000005u,
                /* Expected response not received in time. */
  PAL_STATUS_NOT_IMPLEMENTED        = 0x00004B1Du
                /* API stubbed for now but expected to be implemented later. */
} pal_status_t;

/* Backwards compatibility shim. */
#ifndef NOT_IMPLEMENTED
#define NOT_IMPLEMENTED PAL_STATUS_NOT_IMPLEMENTED
#endif

#define PAL_STATUS_SUCCEEDED(status) \
  ((status) == PAL_STATUS_SUCCESS)

#define PAL_STATUS_FAILED(status) \
  (!PAL_STATUS_SUCCEEDED(status))

#define PAL_STATUS_IS_ERROR(status) \
  PAL_STATUS_FAILED(status)

#define PAL_STATUS_IS_UNSUPPORTED(status) \
  ((status) == PAL_STATUS_UNSUPPORTED)

#define PAL_STATUS_IS_NOT_IMPLEMENTED(status) \
  ((status) == PAL_STATUS_NOT_IMPLEMENTED)

#define PAL_STATUS_IS_RETRYABLE(status) \
  ((status) == PAL_STATUS_BUSY || (status) == PAL_STATUS_TIMEOUT)

#define PAL_STATUS_FROM_FEATURE(supported) \
  ((supported) ? PAL_STATUS_FEATURE_SUPPORTED : PAL_STATUS_FEATURE_UNSUPPORTED)

#define PAL_STATUS_IS_FEATURE_SUPPORTED(status) \
  ((status) == PAL_STATUS_FEATURE_SUPPORTED)

#endif /* PAL_STATUS_H_ */
