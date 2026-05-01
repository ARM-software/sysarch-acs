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

#ifndef __ACS_APP_TABLES_H__
#define __ACS_APP_TABLES_H__

#include "acs_stdint.h"

uint32_t createPeInfoTable(void);
uint32_t createGicInfoTable(void);
uint32_t createRasInfoTable(void);

void     createTimerInfoTable(void);
void     createWatchdogInfoTable(void);
void     createCxlInfoTable(void);
void     createPeripheralInfoTable(void);
void     createSmbiosInfoTable(void);
void     createPmuInfoTable(void);
void     createCacheInfoTable(void);
void     createMpamInfoTable(void);
void     createHmatInfoTable(void);
void     createSratInfoTable(void);
void     createPccInfoTable(void);
void     createRas2InfoTable(void);
void     createTpm2InfoTable(void);

#endif /* __ACS_APP_TABLES_H__ */
