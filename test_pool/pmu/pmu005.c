/** @file
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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

 #include "val/include/acs_val.h"
 #include "val/include/acs_pe.h"
 #include "val/include/acs_common.h"
 #include "val/include/acs_pmu.h"
 #include "val/include/acs_memory.h"
 #include "val/include/acs_mpam.h"
 #include "val/include/val_interface.h"
 #include "val/include/acs_pcie.h"

 #define BW_MON_COUNT 6
 #define LAT_MON_COUNT 4

 #define TEST_NUM  (ACS_PMU_TEST_NUM_BASE + 7)
 #define TEST_RULE "PMU_BM_1"
 #define TEST_DESC "Check for memory bandwidth monitors   "

 #define TEST_NUM1  (ACS_PMU_TEST_NUM_BASE + 10)
 #define TEST_RULE1 "PMU_MEM_1"
 #define TEST_DESC1 "Check for memory latency monitors     "

 #define TEST_NUM2  (ACS_PMU_TEST_NUM_BASE + 11)
 #define TEST_RULE2 "PMU_BM_2"
 #define TEST_DESC2 "Check for PCIe bandwidth monitors     "

/* List of PMU events which can be used to monitor bandwidth */
static PMU_EVENT_TYPE_e bandwidth_mon_event_list[BW_MON_COUNT] = {
    PMU_EVENT_IB_TOTAL_BW,   /* Inbound total bandwidth */
    PMU_EVENT_OB_TOTAL_BW,   /* Outbound total bandwidth */
    PMU_EVENT_IB_READ_BW,    /* Inbound read bandwidth */
    PMU_EVENT_IB_WRITE_BW,   /* Inbound write bandwidth */
    PMU_EVENT_OB_READ_BW,    /* Outbound read bandwidth */
    PMU_EVENT_OB_WRITE_BW,   /* Outbound write bandwidth */
};

/* List of PMU events which can be used to measure average read latency */
static PMU_EVENT_TYPE_e read_latency_mon_event_list[LAT_MON_COUNT] = {
    PMU_EVENT_IB_OPEN_TXN,        /* Inbound open transactions   */
    PMU_EVENT_IB_TOTAL_TXN,       /* Inbound total transactions  */
    PMU_EVENT_OB_OPEN_TXN,        /* Outbound open transactions  */
    PMU_EVENT_OB_TOTAL_TXN,       /* Outbound total transactions */
};

/* Define test data structure to be used to pass payload with parameters */
typedef struct {
    PMU_EVENT_TYPE_e *event_list;
    uint32_t num_events;
    uint32_t interface_type;
    uint32_t test_num;
} test_data_t;

/* This functions checks if counters collectively supports event list passed as parameter */
test_status_t check_event_support(PMU_EVENT_TYPE_e *event_list, uint32_t num_events,
                                  uint64_t node_primary_instance, PMU_NODE_INFO_TYPE node_type)
{
    uint32_t counter, event_idx;
    uint32_t num_counters = 0;
    uint8_t event_supported[num_events];
    uint32_t node_index;

    /* Get PMU node index corresponding to the node instance primary */
    node_index = val_pmu_get_node_index(node_primary_instance, node_type);
    if (node_index == PMU_INVALID_INDEX) {
        val_print(ACS_PRINT_ERR, "\n       Node primary instance : 0x%lx ", node_primary_instance);
        val_print(ACS_PRINT_ERR, "\n       Node type : 0x%x has no PMU associated with it",
                (uint32_t)node_type);
        return TEST_FAIL;
    }

    /* Based on scenario to run check if minimum event counters are met*/
    num_counters = val_pmu_get_monitor_count(node_index);
    if (num_counters <= 0) {
        val_print(ACS_PRINT_ERR, "\n       PMU node must support atleast one counter."
                  , 0);
        return TEST_FAIL;
    }

    /* initialise even_supported array*/
    for (event_idx = 0; event_idx < num_events; event_idx++)
        event_supported[event_idx] = 0;

    /* Check if counters collectively support events required */
    for (counter = 0; counter < num_counters; counter++) {
        for (event_idx = 0; event_idx < num_events; event_idx++) {
            if (event_supported[event_idx])
                continue;

            if (val_pmu_configure_monitor(node_index, event_list[event_idx], counter) == 0) {
                event_supported[event_idx] = 1;
                val_print(ACS_PRINT_DEBUG, "\n       Counter : %d ", counter);
                val_print(ACS_PRINT_DEBUG, "Supports event ID :%d", event_list[event_idx]);
            }
        }
    }

    for (event_idx = 0; event_idx < num_events; event_idx++) {
        if (!event_supported[event_idx]) {
            val_print(ACS_PRINT_DEBUG, "\n       Missing support for event ID %d",
                      event_list[event_idx]);
            return TEST_FAIL;
        }
    }

    return TEST_PASS;
}

/* This payload checks for presence PMU monitor(s) with required event list passed as parameter */
static void payload_check_pmu_monitors(void *arg)
{
    test_status_t status;
    uint64_t num_mem_range, mem_range_index, mc_prox_domain;
    uint64_t node_count, node_index, pcie_rc_id;
    uint32_t fail_cnt = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    uint8_t  run_flag = 0;
    test_data_t *test_data = (test_data_t *)arg;

    /* Check if system has Coresight compatible system PMU */
    status = is_coresight_pmu_present();
    if (status != TEST_PASS) {
        val_set_status(index, TEST_STATUS(test_data->test_num, status, 01));
        return;
    }

    if (test_data->interface_type == PMU_NODE_MEM_CNTR) {
        /* Get number of memory ranges from SRAT table */
        num_mem_range = val_srat_get_info(SRAT_MEM_NUM_MEM_RANGE, 0);
        if (num_mem_range == 0 || num_mem_range == SRAT_INVALID_INFO) {
            val_print(ACS_PRINT_ERR, "\n       No Proximity domains in the system", 0);
            val_set_status(index, RESULT_FAIL(test_data->test_num, 02));
            return;
        }

        /* Loop through the memory ranges listed on SRAT table */
        for (mem_range_index = 0 ; mem_range_index < num_mem_range ; mem_range_index++) {

            /* Get proximity domain mapped to the memory range */
            mc_prox_domain = val_srat_get_prox_domain(mem_range_index);

            /* Check if interface supports required events */
            status = check_event_support(test_data->event_list, test_data->num_events,
                                         mc_prox_domain, test_data->interface_type);

            if (status == TEST_FAIL) {
                fail_cnt++;
            }
        }
    } else if (test_data->interface_type == PMU_NODE_PCIE_RC) {
        node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
        /* Loop through all the PMU nodes and find nodes associated with PCIe root complex */
        for (node_index = 0; node_index < node_count; node_index++) {
            /* Check the PMU nodes which are associated with PCIe RC */
            if (val_pmu_get_info(PMU_NODE_TYPE, node_index) == PMU_NODE_PCIE_RC) {
                /* Get primary node instance for PCIe RC */
                pcie_rc_id = val_pmu_get_info(PMU_NODE_PRI_INST, node_index);
                /* Check if interface supports required events */
                status = check_event_support(test_data->event_list, test_data->num_events,
                                             pcie_rc_id, test_data->interface_type);
                if (status == TEST_FAIL) {
                    fail_cnt++;
                }
                /* mark test run, to report as fail if no PCIe RC entry present
                   APMT ACPI Table*/
                run_flag = 1;
            }
        }
    } else {
        val_print(ACS_PRINT_ERR, "\n       Invalid interface type passed to check_monitors()", 0);
        val_set_status(index, RESULT_FAIL(test_data->test_num, 03));
        return;
    }

    if (!run_flag) {
        val_print(ACS_PRINT_ERR, "\n       No PMU associated with PCIe interface", 0);
        val_set_status(index, RESULT_FAIL(test_data->test_num, 04));
        return;
    }

    if (fail_cnt) {
        val_set_status(index, RESULT_FAIL(test_data->test_num, 05));
        return;
    }

    val_set_status(index, RESULT_PASS(test_data->test_num, 01));
}

uint32_t
pmu007_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    test_data_t data = {.event_list = bandwidth_mon_event_list, .num_events = BW_MON_COUNT,
                        .interface_type = PMU_NODE_MEM_CNTR, .test_num = TEST_NUM};

    num_pe = 1; /* This test is run on a single PE */

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_configurable_payload(&data, payload_check_pmu_monitors);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}

uint32_t
pmu010_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    test_data_t data = {.event_list = read_latency_mon_event_list, .num_events = LAT_MON_COUNT,
                        .interface_type = PMU_NODE_MEM_CNTR, .test_num = TEST_NUM1};

    num_pe = 1; /* This test is run on a single PE */

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_configurable_payload(&data, payload_check_pmu_monitors);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);

    return status;
}

uint32_t
pmu011_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    test_data_t data = {.event_list = bandwidth_mon_event_list, .num_events = BW_MON_COUNT,
        .interface_type = PMU_NODE_PCIE_RC, .test_num = TEST_NUM2};

    num_pe = 1; /* This test is run on a single PE */

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM2, TEST_DESC2, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_configurable_payload(&data, payload_check_pmu_monitors);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM2, num_pe, TEST_RULE2);
    val_report_status(0, ACS_END(TEST_NUM2), TEST_RULE2);

    return status;
}

