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

#define TEST_NUM  (ACS_PMU_TEST_NUM_BASE + 4)
#define TEST_RULE "PMU_SYS_1"
#define TEST_DESC "Test Simultaneous 4 traffic measures  "

#define TEST_NUM1  (ACS_PMU_TEST_NUM_BASE + 5)
#define TEST_RULE1 "PMU_SYS_2"
#define TEST_DESC1 "Test Simultaneous 6 traffic measures  "

#define BUFFER_SIZE 0x400000 /* 4 Megabytes*/
#define PMU_SYS_1_MON_COUNT 6
#define PMU_SYS_2_MON_COUNT 8
#define MAX_MON_COUNT PMU_SYS_2_MON_COUNT

/* Define test scenarios */
typedef enum {
     /* TEST_SCENARIO_PMU_SYS_1
         - Average outbound read latency
         - Total outbound bandwidth
         - Average inbound read latency
         - Total inbound bandwidth */
    TEST_SCENARIO_PMU_SYS_1,
     /* TEST_SCENARIO_PMU_SYS_2
         - Average outbound read latency
         - Read outbound bandwidth
         - Write outbound bandwidth
         - Average inbound read latency
         - Read inbound bandwidth
         - Write inbound bandwidth */
    TEST_SCENARIO_PMU_SYS_2,
    MAX_SCENARIOS /* Max test scenarios */
} TEST_SCENARIO_e;

/* Following system PMU events would be required  to measure average inbound and outband read
latencies and inbound and outbound total bandwidths */
/* TEST_SCENARIO_PMU_SYS_1 */
static PMU_EVENT_TYPE_e scenario_1_events[PMU_SYS_1_MON_COUNT] = {
    PMU_EVENT_IB_TOTAL_BW,   /* Inbound total bandwidth */
    PMU_EVENT_OB_TOTAL_BW,   /* Outbound total bandwidth */
    PMU_EVENT_IB_OPEN_TXN,   /* Inbound open transactions */
    PMU_EVENT_IB_TOTAL_TXN,  /* Inbound total transactions */
    PMU_EVENT_OB_OPEN_TXN,   /* Outbound open transactions */
    PMU_EVENT_OB_TOTAL_TXN,  /* Outbound total transactions */
};

/* Following system PMU events would be required  to measure average inbound and outband read
latencies and inbound and outbound total read/write bandwidths */
/* TEST_SCENARIO_PMU_SYS_2 */
static PMU_EVENT_TYPE_e scenario_2_events[PMU_SYS_2_MON_COUNT] = {
    PMU_EVENT_IB_READ_BW,    /* Inbound read bandwidth */
    PMU_EVENT_IB_WRITE_BW,   /* Inbound write bandwidth */
    PMU_EVENT_OB_READ_BW,    /* Outbound read bandwidth */
    PMU_EVENT_OB_WRITE_BW,   /* Outbound write bandwidth */
    PMU_EVENT_IB_OPEN_TXN,   /* Inbound open transactions */
    PMU_EVENT_IB_TOTAL_TXN,  /* Inbound total transactions */
    PMU_EVENT_OB_OPEN_TXN,   /* Outbound open transactions */
    PMU_EVENT_OB_TOTAL_TXN,  /* Outbound total transactions */
};

/* Define a structure to store monitor values taken at time t1 and t2 */
typedef struct {
    uint64_t t1_value;
    uint64_t t2_value;
} MONITOR_VALUE;

/* Define test data structure to be used to pass payload with parameters */
typedef struct {
    TEST_SCENARIO_e scenario;
    uint32_t test_num;
} test_data_t;

/* Generates Inbound read/write traffic at memory interface */
static uint32_t generate_mem_traffic(uint64_t base_addr, uint32_t size)
{
    void *src_buf = 0;
    void *dest_buf = 0;

    /* Allocate memory for source and destination buffer */
    src_buf = (void *)val_mem_alloc_at_address(base_addr, size);
    dest_buf = (void *)val_mem_alloc_at_address(base_addr + size, size);

    if ((src_buf == NULL) || (dest_buf == NULL))
        return 1;

    /* Since we want transfer to happen from memory to memory, lets ensure we are not
    using cached values */
    val_pe_cache_clean_invalidate_range((uint64_t)src_buf, (uint64_t)size);
    val_pe_cache_clean_invalidate_range((uint64_t)dest_buf, (uint64_t)size);

    /* Perform memory copy for given size */
    val_memcpy(src_buf, dest_buf, size);

    /* Free the buffers */
    val_mem_free_at_address((uint64_t)src_buf, size);
    val_mem_free_at_address((uint64_t)dest_buf, size);
    return 0;
}

/* Generate PCIE traffic, takes max of device ids to target and number of ecams as input  */
static void generate_pcie_traffic(uint32_t num_ecam, uint32_t max_dev)
{
    uint32_t ecam_index;
    uint32_t start_bus, seg_num;
    uint32_t dev_index, func_index;
    uint32_t bdf, reg_value;

    for (ecam_index = 0; ecam_index < num_ecam; ecam_index++)
    {
        /* Derive ecam specific information */
        seg_num = val_pcie_get_info(PCIE_INFO_SEGMENT, ecam_index);
        start_bus = val_pcie_get_info(PCIE_INFO_START_BUS, ecam_index);

        /* Iterate over all buses, devices and functions in this ecam */
        for (dev_index = 0; dev_index < max_dev; dev_index++) {
            for (func_index = 0; func_index < PCIE_MAX_FUNC; func_index++) {
                /* Form bdf using seg, bus, device, function numbers */
                bdf = PCIE_CREATE_BDF(seg_num, start_bus, dev_index, func_index);
                val_pcie_read_cfg(bdf, TYPE01_VIDR, &reg_value);
                val_pcie_write_cfg(bdf, TYPE01_VIDR, 0xFFFFFFFF);
            }
        }
    }
}

/* This routine tests the scenario for the specified node type and primary instance,
   as listed in the APMT ACPT table. */
test_status_t sys_pmu_test_scenario(TEST_SCENARIO_e scenario, PMU_NODE_INFO_TYPE node_type,
                                     uint64_t node_primary_instance)
{
    uint32_t node_index;
    uint32_t status;
    uint64_t base_addr, addr_len;
    uint32_t i;
    uint64_t data = 0;
    uint32_t min_monitor_cnt;
    PMU_EVENT_TYPE_e *event_ptr;
    uint64_t num_ecam;
    /* Declare the array to hold monitor values */
    MONITOR_VALUE monitor_data[MAX_MON_COUNT];

    /* Based on scenario passed decide on minimum number of PMU monitors required and
    event list */
    if (scenario == TEST_SCENARIO_PMU_SYS_1) {
        min_monitor_cnt = PMU_SYS_1_MON_COUNT;
        event_ptr = scenario_1_events;
    } else if (scenario == TEST_SCENARIO_PMU_SYS_2) {
        min_monitor_cnt = PMU_SYS_2_MON_COUNT;
        event_ptr = scenario_2_events;
    } else {
        val_print(ACS_PRINT_ERR, "\n       Undefined scenario passed to sys_pmu_test_scenario ()",
                0);
        return TEST_FAIL;
    }

    /* Validate node_type passed */
    if (node_type != PMU_NODE_MEM_CNTR && node_type != PMU_NODE_PCIE_RC) {
        val_print(ACS_PRINT_ERR, "\n       Invalid node_type passed to sys_pmu_test_scenario ()",
                0);
        return TEST_FAIL;
    }

    /* Get PMU node index corresponding to the node instance primary */
    node_index = val_pmu_get_node_index(node_primary_instance, node_type);
    if (node_index == PMU_INVALID_INDEX) {
        val_print(ACS_PRINT_ERR, "\n       Node primary instance : 0x%lx ",
                node_primary_instance);
        val_print(ACS_PRINT_ERR, "\n       Node type : 0x%x has no PMU associated with it",
                (uint32_t)node_type);
        return TEST_FAIL;
    }

    /* Based on scenario to run check if minimum event counters are met*/
    data = val_pmu_get_monitor_count(node_index);
    if (data < min_monitor_cnt) {
        val_print(ACS_PRINT_ERR, "\n       PMU node must support atleast %d counter for "
                    "\n       measuring current scenario ", min_monitor_cnt);
        return TEST_FAIL;
    }

    /* If node_type is memory controller fetch the base address */
    if (node_type == PMU_NODE_MEM_CNTR) {
        /* Get base address of the proximity domain */
        base_addr = val_srat_get_info(SRAT_MEM_BASE_ADDR, node_primary_instance);
        addr_len = val_srat_get_info(SRAT_MEM_ADDR_LEN, node_primary_instance);
        if ((base_addr == SRAT_INVALID_INFO) || (addr_len == SRAT_INVALID_INFO) ||
            (addr_len <= 2 * BUFFER_SIZE)) {
            val_print(ACS_PRINT_ERR,
                        "\n       Invalid base address for proximity domain : 0x%lx",
                        node_primary_instance);
            return TEST_FAIL;
        }
    /* If PCIe fetch the */
    } else if (node_type == PMU_NODE_PCIE_RC) {
        num_ecam = val_pcie_get_info(PCIE_INFO_NUM_ECAM, 0);
        if (num_ecam == 0) {
            /* If APMT has PCIE RC entry we shouldn't have entered this if(), if we
            did, something's wrong with system information */
            val_print(ACS_PRINT_ERR, "\n       No ECAMs reported by system", 0);
            return TEST_FAIL;
        }
    }

    /* Configure PMEVTYPER with available PMU counters available, we check the counters
        if it supports required event types if it doesn't we fail the test */
    for (i = 0; i < min_monitor_cnt; i++) {
        status = val_pmu_configure_monitor(node_index, event_ptr[i], i);
        if (status) {
            val_print(ACS_PRINT_ERR,
                        "\n       Required PMU Event 0x%x not supported", event_ptr[i]);
            val_print(ACS_PRINT_ERR, " at node %d", node_index);
            return TEST_FAIL;
        }
    }

    /* Enable all required monitors */
    for (i = 0; i < min_monitor_cnt ; i++)
        val_pmu_enable_monitor(node_index, i);

    /* Generate traffic (t1)*/
    if (node_type == PMU_NODE_MEM_CNTR) {
        status = generate_mem_traffic(base_addr, BUFFER_SIZE / 2);
        if (status) {
            val_print(ACS_PRINT_ERR, "\n       Memory allocation failed", node_index);
            val_pmu_disable_all_monitors(node_index);
            return TEST_FAIL;
        }
    } else if (node_type == PMU_NODE_PCIE_RC) {
        generate_pcie_traffic(num_ecam, PCIE_MAX_DEV/2);
    }

    /* Read and store the monitor values for later comparison */
    for (i = 0; i < min_monitor_cnt; ++i) {
        monitor_data[i].t1_value = val_pmu_read_count(node_index, i);
    }

    /* Reset the monitors */
    for (i = 0; i < min_monitor_cnt ; i++) {
        val_pmu_disable_monitor(node_index, i);
        val_pmu_enable_monitor(node_index, i);
    }

    /* Generate traffic (t1)*/
    if (node_type == PMU_NODE_MEM_CNTR) {
        status = generate_mem_traffic(base_addr, BUFFER_SIZE);
        if (status) {
            val_print(ACS_PRINT_ERR, "\n       Memory allocation failed", node_index);
            val_pmu_disable_all_monitors(node_index);
            return TEST_FAIL;
        }
    } else if (node_type == PMU_NODE_PCIE_RC) {
        generate_pcie_traffic(num_ecam, PCIE_MAX_DEV);
    }

    /* Read and store the monitor values for later comparison */
    for (i = 0; i < min_monitor_cnt; ++i) {
        monitor_data[i].t2_value = val_pmu_read_count(node_index, i);
    }

    /* Consider delta to check if counter is moving in proportion, and for all the event
        monitor values taken for 4MB transfer should be greater than for 2MB transfer  */
    for (i = 0; i < min_monitor_cnt ; i++) {
        /* Check for FAIL */
        if (monitor_data[i].t1_value > monitor_data[i].t2_value) {
            val_print(ACS_PRINT_ERR, "\n       Monitors didn't move in expected proportions"
                    " for PMU node index: 0x%x",
                    node_index);
            /* Disable PMU monitors */
            val_pmu_disable_all_monitors(node_index);
            return TEST_FAIL;

        }
    }

    /* Disable PMU monitors */
    val_pmu_disable_all_monitors(node_index);
    return TEST_PASS;
}

/* This payload executes scenario as per passed argument */
static void payload_check_sys_pmu_scenario(void *arg)
{
    uint32_t fail_cnt = 0;
    uint32_t node_count;
    uint32_t node_index;
    uint64_t mem_range_index;
    uint64_t num_mem_range;
    uint64_t mc_prox_domain;
    uint8_t run_flag = 0;
    test_status_t status;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
    test_data_t *test_data = (test_data_t *)arg;

    /* Check if system has Coresight compatible system PMU */
    status = is_coresight_pmu_present();

    if (status != TEST_PASS) {
        val_set_status(index, TEST_STATUS(test_data->test_num, status, 01));
        return;
    }

    /* Monitor measurements for memory interface */

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
        if (mc_prox_domain == SRAT_INVALID_INFO) {
            val_print(ACS_PRINT_ERR, "\n       Proximity domain not found", 0);
            fail_cnt++;
            continue;
        }

        /* Test for scenario TEST_SCENARIO_PMU_SYS_1 for memory controller */
        status = sys_pmu_test_scenario(test_data->scenario, PMU_NODE_MEM_CNTR, mc_prox_domain);
        if (status == TEST_FAIL) {
            fail_cnt++;
        }
    }

    /* Loop through all the PMU nodes and find nodes associated with PCIe root complex */
    node_count = val_pmu_get_info(PMU_NODE_COUNT, 0);
    for (node_index = 0; node_index < node_count; node_index++) {
        /* Check the PMU nodes which are associated with PCIe RC */
        if (val_pmu_get_info(PMU_NODE_TYPE, node_index) == PMU_NODE_PCIE_RC) {
            /* Test for scenario TEST_SCENARIO_PMU_SYS_1 for PCIe root complex */
            status = sys_pmu_test_scenario(test_data->scenario, PMU_NODE_PCIE_RC,
                                        val_pmu_get_info(PMU_NODE_PRI_INST, node_index));
            if (status == TEST_FAIL) {
                fail_cnt++;
            }
            /* mark test run, to report as skip if no PCIe RC entry present in APMT ACPI Table*/
            run_flag = 1;
        }
    }

    if (!run_flag) {
        val_print(ACS_PRINT_ERR, "\n       No PMU associated with PCIe interface", 0);
        val_set_status(index, RESULT_FAIL(test_data->test_num, 03));
        return;
    }

    if (fail_cnt) {
        val_set_status(index, RESULT_FAIL(test_data->test_num, 04));
        return;
    }

    val_set_status(index, RESULT_PASS(test_data->test_num, 01));
}

uint32_t pmu004_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    test_data_t data = {.scenario = TEST_SCENARIO_PMU_SYS_1, .test_num = TEST_NUM};

    num_pe = 1; /* This test is run on a single PE */

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_configurable_payload(&data, payload_check_sys_pmu_scenario);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}

uint32_t pmu005_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;
    test_data_t data = {.scenario = TEST_SCENARIO_PMU_SYS_2, .test_num = TEST_NUM1};


    num_pe = 1; /* This test is run on a single PE */

    val_log_context((char8_t *)__FILE__, (char8_t *)__func__, __LINE__);
    status = val_initialize_test(TEST_NUM1, TEST_DESC1, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_configurable_payload(&data, payload_check_sys_pmu_scenario);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM1, num_pe, TEST_RULE1);
    val_report_status(0, ACS_END(TEST_NUM1), TEST_RULE1);

    return status;
}
