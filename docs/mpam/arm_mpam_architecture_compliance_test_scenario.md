# Arm MPAM MSC Architecture Compliance Test Scenarios

**Specification**: MPAM Memory System Component Specification [(ARM IHI 0099A.a)](https://developer.arm.com/documentation/ihi0099/aa/)

---

## Table of contents
- [Arm MPAM MSC Architecture](#arm-mpam-msc-architecture)
  - [SYS-MPAM ACS](#sys-mpam-acs)
  - [Register Tests](#register-tests)
    - [Test 1 Check MPAM Version EXT Bit Check](#test-1-check-mpam-version-ext-bit-check)
    - [Test 2 Check Expansion of MPAMF_ESR](#test-2-check-expansion-of-mpamf_esr)
    - [Test 3 Check MPAM MSC Feature Test](#test-3-check-mpam-msc-feature-test)
    - [Test 4 Check MPAM for RME Feature Test](#test-4-check-mpam-for-rme-feature-test)
    - [Test 5 Check MSC Four Space Region Test](#test-5-check-msc-four-space-region-test)
    - [Test 6 Check NFU bit when PARTID NRW is impl](#test-6-check-nfu-bit-when-partid-nrw-is-impl)
  - [Cache Tests](#cache-tests)
    - [Test 101 Check CPOR Partitioning](#test-101-check-cpor-partitioning)
    - [Test 102 Check CCAP Partitioning](#test-102-check-ccap-partitioning)
    - [Test 103 Check CPOR with CCAP Partitioning](#test-103-check-cpor-with-ccap-partitioning)
    - [Test 104 Check CASSOC Partitioning](#test-104-check-cassoc-partitioning)
    - [Test 105 Check CMAX Partitioning with softlimit](#test-105-check-cmax-partitioning-with-softlimit)
    - [Test 106 Check CMIN resource controls](#test-106-check-cmin-resource-controls)
    - [Test 125 Check PARTID Disable functionality](#test-125-check-partid-disable-functionality)
  - [Monitor Tests](#monitor-tests)
    - [Test 151 Check PMG Storage by CPOR Nodes](#test-151-check-pmg-storage-by-cpor-nodes)
    - [Test 152 Check PMG Storage by CCAP Nodes](#test-152-check-pmg-storage-by-ccap-nodes)
    - [Test 153 Check PARTID Storage by CPOR Nodes](#test-153-check-partid-storage-by-cpor-nodes)
    - [Test 154 Check PARTID Storage by CCAP Nodes](#test-154-check-partid-storage-by-ccap-nodes)
    - [Test 155 Check MSMON CTL Disable Behavior](#test-155-check-msmon-ctl-disable-behavior)
  - [Error Tests](#error-tests)
    - [Test 201 Check MSC PARTID selection range error](#test-201-check-msc-partid-selection-range-error)
    - [Test 202 Check request PARTID out-of-range error](#test-202-check-request-partid-out-of-range-error)
    - [Test 203 Check MSMON config ID out-of-range error](#test-203-check-msmon-config-id-out-of-range-error)
    - [Test 204 Check request PMG out-of-range error](#test-204-check-request-pmg-out-of-range-error)
    - [Test 205 Check MSC MONITOR selection range error](#test-205-check-msc-monitor-selection-range-error)
    - [Test 206 Check intPARTID out-of-range error](#test-206-check-intpartid-out-of-range-error)
    - [Test 207 Check Unexpected internal error](#test-207-check-unexpected-internal-error)
    - [Test 208 Check undefined RIS in MPAMCFG_PART_SEL error](#test-208-check-undefined-ris-in-mpamcfg_part_sel-error)
    - [Test 209 Check RIS no control error](#test-209-check-ris-no-control-error)
    - [Test 210 Check undefined RIS in MSMON_CFG_MON_SEL error](#test-210-check-undefined-ris-in-msmon_cfg_mon_sel-error)
    - [Test 211 Check RIS no monitor error](#test-211-check-ris-no-monitor-error)
    - [Test 212 Check MSC Level-Sensitive Error Interrupt Behaviour](#test-212-check-msc-level-sensitive-error-interrupt-behaviour)
    - [Test 213 Check MSC Edge-Trigger Error Interrupt Behaviour](#test-213-check-msc-edge-trigger-error-interrupt-behaviour)
    - [Test 214 Check MBWU monitor overflow interrupt functionality](#test-214-check-mbwu-monitor-overflow-interrupt-functionality)
    - [Test 215 Check MPAM IDR Zero for Undefined RIS](#test-215-check-mpam-idr-zero-for-undefined-ris)
    - [Test 216 Check MPAMF_ESR.RIS field correctness](#test-216-check-mpamf_esr-ris-field-correctness)
    - [Test 217 Check MPAM error overwrite behavior](#test-217-check-mpam-error-overwrite-behavior)
    - [Test 218 Check MBWU_L overflow interrupt status](#test-218-check-mbwu_l-overflow-interrupt-status)
    - [Test 219 Check MBWU Oflow MSI interrupt status](#test-219-check-mbwu-oflow-msi-interrupt-status)
    - [Test 220 Check MPAM error MSI interrupt status](#test-220-check-mpam-error-msi-interrupt-status)
  - [Memory Bandwidth Tests](#memory-bandwidth-tests)
    - [Test 301 Check MBWPBM Partitioning](#test-301-check-mbwpbm-partitioning)
    - [Test 302 Check MBWMIN Partitioning](#test-302-check-mbwmin-partitioning)
    - [Test 303 Check MBWMAX Partitioning](#test-303-check-mbwmax-partitioning)
    - [Test 304 Check MBWU MSMON CTL Disable behavior](#test-304-check-mbwu-msmon-ctl-disable-behavior)
    - [Test 305 Check MBWU OFLOW_FRZ overflow behavior](#test-305-check-mbwu-oflow_frz-overflow-behavior)
    - [Test 306 Check MBWU MSMON OFLOW Reset behavior](#test-306-check-mbwu-msmon-oflow-reset-behavior)
- [Appendix A. Revisions](#appendix-a-revisions)

---

## Arm MPAM MSC Architecture
The MPAM (Memory System Resource Partitioning and Monitoring) architecture is designed for systems running multiple applications or virtual machines on shared memory, particularly in enterprise networking and server environments. It addresses key requirements like controlling the performance impact of non-conforming software, bounding resource usage, and minimizing interference between software components.

MPAM achieves this through two main mechanisms: memory-system resource partitioning and resource usage monitoring. It propagates Partition IDs (PARTID) and Performance Monitoring Groups (PMG) across the memory system, enabling fine-grained control and monitoring of memory resources.

MPAM defines a framework for memory-system components (MSCs) to manage and monitor resource usage, providing standardized, implementation-independent memory-mapped interfaces. These features help align system performance with higher-level software goals and ensure consistent behaviour in complex, multi-tenant environments.

The goal of SYS-MPAM ACS is to test the errors in MSCs, partitioning, and monitoring related features for compliance against the MPAM MSC specification.

### SYS-MPAM ACS
The tests are classified as:
- Register Tests
- Cache Tests
- Monitor Tests
- Error Tests
- Memory Bandwidth Tests

---

### Register Tests

#### Test 1 Check MPAM Version EXT Bit Check
**Scenario**: Validate the EXT field for supported MPAM versions.

**Algorithm**:
- Get the total number of MSC nodes.
- Loop through each MSC node by index.
- If version is MPAM v1.0, check if EXT field is 0.
- If version is MPAM v1.1, check if EXT field is 1.
- If EXT field doesn’t match the expected value, print error and fail the test.
- If version is not 1.0 or 1.1, print error and fail the test.
- If all MSCs pass the check, mark the test as passed.

---

#### Test 2 Check Expansion of MPAMF_ESR
**Scenario**: Validate HAS_EXTD_ESR when HAS_ESR, HAS_RIS, and EXT are set.

**Algorithm**:
- Get the total number of MSC nodes.
- Loop through each MSC node by index.
- Read the MPAMF_IDR register value.
- If both HAS_ESR and HAS_RIS are set, and EXT is set:
  - Check if HAS_EXTD_ESR is also set.
  - If HAS_EXTD_ESR is not set, print error and fail the test.
- If all MSCs pass the check, mark the test as passed.

---

#### Test 3 Check MPAM MSC Feature Test
**Scenario**: Validate MPAM MSC feature fields based on supported versions.

**Algorithm**:
- Get the total number of MSC nodes.
- Loop through each MSC node by index.
- Read ID registers: MPAMF_IDR, CCAP_IDR, CSUMON_IDR, MBWUMON_IDR, MSMON_IDR.
- If version is MPAM v1.0:
  - Check that all prohibited fields (for example, HAS_CMIN, NO_CMAX, HAS_CASSOC, HAS_ENDIS) are not set.
  - If any prohibited field is set, print error and increment test failure counter.
- If version is MPAM v1.1 or v0.1:
  - If HAS_IMPL_IDR is set, ensure required fields (NO_IMPL_PART, NO_IMPL_MSMON) are also set.
  - If not, print error and increment failure counter.
  - Check that EXT field is set; if not, print error and increment failure counter.
- If version is unsupported, print error and fail the test.
- After all nodes are checked, if any failure was recorded, mark the test as failed. Otherwise, mark the test as passed.

---

#### Test 4 Check MPAM for RME Feature Test
**Scenario**: Verify MPAM behavior when RME is present.

**Algorithm**:
- For each PE, read the ID_AA64PFR0_EL1 system register.
- Check if ID_AA64PFR0_EL1.RME field is set (indicates RME support) and ID_AA64PFR0_EL1.MPAM is set.
- Check if MPAM support is at least v1.1 (ID_AA64PFR0_EL1.MPAM == 0b0001 && ID_AA64PFR1_EL1.MPAM_frac == 0b0001).
- Confirm the PE supports four MPAM PARTID spaces (MPAMIDR_EL1.SP4 == 1).
- Verify the ALTSP feature is implemented ( MPAMIDR_EL1.HAS_ALTSP == 1).
- If any condition fails, log an error.
- If all PEs with RME meet the above criteria, mark the test as passed.

---

#### Test 5 Check MSC Four Space Region Test
**Scenario**: Verify at least one MSC advertises four-space regions when MPAM for RME is supported.

**Algorithm**:
- Identify all MSC components in the system.
- For each MSC, read the MPAMF_IDR.SP4 bit.
- Confirm that at least one MSC has SP4 == 1, indicating it supports four MPAM PARTID spaces.
- If no MSC has SP4 == 1, log an error and fail the test.
- Otherwise, confirm that the system includes at least one four-space MPAM region and mark the test as passed.

---

#### Test 6 Check NFU bit when PARTID NRW is impl
**Scenario**: Ensure NFU is not implemented when PARTID narrowing is supported.

**Algorithm**:
- Iterate MSC nodes and detect PARTID narrowing support.
- For nodes with PARTID narrowing, read MPAMF_IDR and ensure HAS_NFU is 0.
- Fail if any node reports HAS_NFU when PARTID narrowing is supported.

---

### Cache Tests

#### Test 101 Check CPOR Partitioning
**Scenario**: Verify CPOR partitioning behaviour using CSU monitor counts.

**Algorithm**:
- Get the index of the LLC and its cache identifier; skip the test if LLC info is invalid or not found.
- For each MSC node:
  - Find matching cache resource nodes of type PE_CACHE.
  - Check if CPOR and CSU monitor are supported. Track number of CPOR-capable nodes and CSU monitor count.
  - Determine max cache size and min/max PARTID.
- Skip the test if CPOR nodes or CSU monitors are not available.
- Allocate memory for storing CSU monitor counters for each CPOR scenario. Read and modify MPAM2_EL2 to set PARTID and PMG.
- For each MSC node with CPOR support:
  - Configure CPOR settings and disable CCAP partition settings.
  - Allocate source and destination buffers.
  - Configure and enable CSU monitor.
  - Wait for NRDY timeout to stabilize configuration.
  - Capture CSU monitor count before and after memory copy.
  - Store the count difference for analysis. Free allocated buffers.
- Restore original MPAM2_EL2 value.
- Compare CSU monitor counters across scenarios. If counter increases when it should decrease, mark test as failed.
- Set test result to pass if all comparisons are valid, else fail.

---

#### Test 102 Check CCAP Partitioning
**Scenario**: Verify CCAP partitioning behaviour using CSU monitor counts.

**Algorithm**:
- Get the index of the LLC and its cache identifier; skip the test if LLC info is invalid or not found.
- For each MSC node:
  - Find matching cache resource nodes of type PE_CACHE.
  - Check if CCAP and CSU monitor are supported.
  - Track number of CCAP-capable nodes and CSU monitor count.
- Determine max cache size and min/max PARTID. Skip the test if CCAP nodes or CSU monitors are not available.
- Allocate memory for storing CSU monitor counters for each CCAP scenario. Read and modify MPAM2_EL2 to set PARTID and PMG.
- For each MSC node with CCAP support: configure CCAP settings and disable CPOR partition settings.
- Allocate source and destination buffers.
- Configure and enable CSU monitor. Wait for NRDY timeout to stabilize configuration.
- Capture CSU monitor count before and after memory copy. Store the count difference for analysis.
- Free allocated buffers. Restore original MPAM2_EL2 value.
- Compare CSU monitor counters across scenarios. If counter increases when it should decrease, mark test as failed.
- Set test result to pass if all comparisons are valid, else fail.

---

#### Test 103 Check CPOR with CCAP Partitioning
**Scenario**: Validate combined CPOR and CCAP partitioning behaviour using CSU monitor counts.

**Algorithm**:
- Get the LLC index and cache identifier; skip the test if either is missing or invalid.
- Loop through all MSC nodes and their resources to find PE_CACHE resources that match the LLC identifier and support both CCAP and CPOR; count valid nodes, update cache max size, track CSU monitor count if supported, and update minimum max PARTID value.
- Skip the test if there are no CCAP+CPOR nodes or no CSU monitors available.
- Allocate a 2D buffer to store CSU monitor counter values for each test scenario and MSC node.
- Read and store the current MPAM2_EL2 value, clear the PARTID_D and PMG_D fields, set the desired PARTID and PMG values, and write the updated value back to MPAM2_EL2.
- For each enabled test scenario, loop through all MSC nodes, and for each matching PE_CACHE resource:
  - Select the resource instance if RIS is supported.
  - Configure CCAP and CPOR partitioning according to the scenario (or fallback to 100% if only one is supported).
  - Allocate source and destination memory buffers.
  - Configure and enable the CSU monitor.
  - Wait for NRDY timeout to complete.
  - Record the start CSU count, perform memory copy, and record the end count.
  - Disable the monitor, store the count difference, and free the allocated memory.
- Restore the original MPAM2_EL2 register value.
- Compare CSU monitor count differences across scenarios for each MSC node; if a later scenario shows more activity than the previous one, log the failure.
- Set the final test status as pass if all comparisons succeed or fail if any mismatch is found.

---

#### Test 104 Check CASSOC Partitioning
**Scenario**: Validate cache associativity partitioning enforcement.

**Algorithm**:
- Identify an MSC with cache resources that support associativity partitioning by checking that MPAMF_IDR.HAS_CCAP_PART is set and MPAMF_CCAP_IDR.HAS_CASSOC is also set.
- Select a valid PARTID, for example PARTID_X.
- Configure MPAMCFG_CASSOC for PARTID_X to 100% cache associativity, which is typically represented by the value 0xFFFF.
- Assign PARTID_X to the running PE using MPAM2_EL2.
- Generate a memory workload from the PE to populate the cache and observe full usage of available ways.
- Measure the cache usage using the CSU monitor and record the value.
- Reconfigure MPAMCFG_CASSOC for PARTID_Y to restrict associativity to 50%, represented by a value like 0x7FFF.
- Repeat the same memory workload under the same conditions.
- Observe whether cache lines allocated to PARTID_X are now limited to fewer ways or if evictions occur earlier.
- Use the CSU monitor to confirm the reduced cache allocation compared to the full associativity setting.
- If the observed cache usage aligns with the restricted CASSOC setting, confirming enforcement of associativity limits, mark the test passed.
- If the allocation behavior remains unchanged despite the restriction, indicating that the control is not being applied, mark the test failed.

---

#### Test 105 Check CMAX Partitioning with softlimit
**Scenario**: Validate CMAX soft limit behavior for cache partitioning.

**Algorithm**:
- Identify an MSC that supports cache maximum-capacity control and soft limit functionality by verifying that MPAMF_IDR.HAS_CCAP_PART, MPAMF_CCAP_IDR.HAS_CMAX, and MPAMF_CCAP_IDR.HAS_SOFTLIM are all set.
- Select two PARTIDs, PARTID_X and PARTID_Y.
- Configure MPAMCFG_CMAX such that PARTID_X is assigned 75% of the cache and PARTID_Y is assigned 25%, both with SOFTLIM = 0.
- Program the PE with PARTID_X using MPAM2_EL2.
- Generate a memory workload under PARTID_X to fully utilize its 75% cache share.
- Measure cache usage using the CSU monitor and record it as Value A.
- Program the PE with PARTID_Y using MPAM2_EL2 and disable it by writing to MPAMCFG_DIS.PARTID (do not set NFU).
- Re-program the PE with PARTID_X again and set SOFTLIM = 1 for PARTID_X.
- Perform the same memory workload from the PE.
- Measure cache usage again with the CSU monitor and record it as Value B.
- Compare Value B with Value A.
- If Value B > Value A, it confirms that PARTID_X was able to use the additional 25% cache that was previously assigned to PARTID_Y, indicating correct SOFTLIM behavior.
- If Value B is not significantly greater than Value A, the test fails.

---

#### Test 106 Check CMIN resource controls
**Scenario**: Validate cache minimum-capacity resource control behavior.

**Algorithm**:
- Identify an MSC that supports Cache Minimum-Capacity resource control by checking both MPAMF_IDR.HAS_CCAP_PART and MPAMF_CCAP_IDR.HAS_CMIN are set.
- Select a valid PARTID (for example, PARTID_X) and configure MPAMCFG_CMAX to allocate a max of 100% of the cache capacity to this PARTID.
- Perform a memory workload that utilizes the cache to ensure that PARTID_X is actively using its allocated cache capacity.
- Select another PARTID (PARTID_Y) and configure MPAMCFG_CMAX to allocate a max of 50% of the cache capacity to it.
- Perform a memory workload that utilizes the cache to ensure that PARTID_Y is actively using its allocated cache capacity.
- Disable PARTID_Y by writing its ID to MPAMCFG_DIS.PARTID without setting the NFU bit, effectively marking its cache lines as highest priority for eviction.
- Select a third PARTID (PARTID_Z) and configure MPAMCFG_CMIN to request 75% of the cache capacity.
- Perform a memory workload that utilizes the cache to ensure that PARTID_Z is actively trying to use the cache.
- Since cache usage by PARTID_Z is below its CMIN threshold, it should be given elevated allocation priority.
- During cache pressure, the system is expected to evict cache lines from the disabled PARTID_Y first, followed by evictions from PARTID_X.
- Use the CSU monitor to observe and confirm that cache usage increases for PARTID_Z while eviction or reduction in cache usage occurs for PARTID_X and disabled PARTID_Y.
- If this behavior is observed, the test passes as it confirms that minimum-capacity rules and priority adjustments are correctly enforced.
- If PARTID_Z does not receive allocation priority or if evictions do not follow the expected order, the test fails.

---

#### Test 125 Check PARTID Disable functionality
**Scenario**: Validate PARTID disable/enable behavior.

**Algorithm**:
- Check if the MSC supports PARTID enable/disable control by confirming MPAMF_IDR.HAS_ENDIS == 1.
- Select a valid PARTID (for example, PARTID_X) and identify a cache resource that supports portion-based partitioning.
- Program PARTID_X with a defined partition share (for example, 100%) using the cache’s CPOR configuration.
- Set MPAM2_EL2 (or equivalent register) to assign PARTID_X to the active PE or thread.
- Perform a memory workload (for example, memcpy or streaming load/store) large enough to utilize a measurable portion of the cache.
- Use the CSU monitor to measure the cache usage by PARTID_X. Record the CSU counter as Value A.
- Disable PARTID_X by writing its ID to MPAMCFG_DIS.PARTID. Do not set the NFU bit.
- Re-run the same memory workload from the same PE.
- Generate traffic for PARTID_Y with CPOR (100%).
- Use the CSU monitor to measure the cache usage by PARTID_X. Record the CSU counter as Value B. Value B should be less than Value A and gradually decreasing.
- Now, re-enable PARTID_X by writing it to MPAMCFG_EN.PARTID and disable PARTID_Y.
- Re-run the same memory workload once more from the same PE.
- Measure cache usage again using CSU. Record this as Value C.
- Validate the results:
  - Value A ≈ Value C confirms that the original partition settings were preserved after re-enabling.
  - Value B < Value A confirms that the disabled PARTID was deprioritized or blocked from allocation.
  - If both conditions hold, the test is passed.

---

### Monitor Tests

#### Test 151 Check PMG Storage by CPOR Nodes
**Scenario**: Validate PMG storage behaviour for CPOR-capable nodes.

**Algorithm**:
- Get the LLC index and cache identifier; skip the test if either is missing or invalid.
- Loop through all MSC nodes and their resources to find PE_CACHE resources matching the LLC identifier that support CPOR; for each valid node:
  - Configure RIS if supported.
  - Update the max cache size.
  - Get the max PMG value.
  - Count CSU monitors if supported.
  - Track the minimum max PARTID value.
- Skip the test if no CPOR-capable nodes or CSU monitors are available.
- Loop through MSC nodes again and for each valid CPOR-capable PE_CACHE resource, configure CPOR partitioning to 75% using the selected PARTID.
- Set up two PMG groups: PMG1 as default and PMG2 as the highest supported PMG. Read and store the current MPAM2_EL2 value, clear PARTID_D and PMG_D fields, set PARTID and PMG1, and write back the modified MPAM2_EL2.
- For each matching PE_CACHE resource, allocate source and destination memory buffers sized to 50% of the cache size, log the buffer size, and skip if allocation fails.
- Configure CSU monitor with PMG1, enable monitoring, wait for NRDY timeout, perform a memory copy transaction, and record the updated CSU monitor count (storage_value1).
- Reconfigure CSU monitor with PMG2, enable monitoring, wait for NRDY timeout again, perform a second memory copy transaction, and record the updated CSU monitor count (storage_value2).
- Disable CSU monitor, restore original MPAM2_EL2 value, and free the memory buffers.
- Fail the test if storage_value1 is zero (no activity recorded for PMG1) or if storage_value2 is non-zero (unexpected activity for PMG2); otherwise, mark the test as passed.

---

#### Test 152 Check PMG Storage by CCAP Nodes
**Scenario**: Validate PMG storage behaviour for CCAP-capable nodes.

**Algorithm**:
- Get the LLC index and cache identifier; skip the test if either is missing or invalid.
- Loop through all MSC nodes and their resources to find PE_CACHE resources matching the LLC identifier that support CCAP; for each valid node:
  - Configure RIS if supported.
  - Update the max cache size.
  - Get the max PMG value.
  - Count CSU monitors if supported.
  - Track the minimum max PARTID value.
- Skip the test if no CCAP-capable nodes or CSU monitors are available.
- Loop through MSC nodes again and for each valid CCAP-capable PE_CACHE resource, configure CCAP partitioning to 75% using the selected PARTID.
- Set up two PMG groups: PMG1 as default and PMG2 as the highest supported PMG. Read and store the current MPAM2_EL2 value, clear PARTID_D and PMG_D fields, set PARTID and PMG1, and write back the modified MPAM2_EL2.
- For each matching PE_CACHE resource, allocate source and destination memory buffers sized to 50% of the cache size, log the buffer size, and skip if allocation fails.
- Configure CSU monitor with PMG1, enable monitoring, wait for NRDY timeout, perform a memory copy transaction, and record the updated CSU monitor count (storage_value1).
- Reconfigure CSU monitor with PMG2, enable monitoring, wait for NRDY timeout again, perform a second memory copy transaction, and record the updated CSU monitor count (storage_value2).
- Disable CSU monitor, restore original MPAM2_EL2 value, and free the memory buffers.
- Fail the test if storage_value1 is zero (no activity recorded for PMG1) or if storage_value2 is non-zero (unexpected activity for PMG2); otherwise, mark the test as passed.

---

#### Test 153 Check PARTID Storage by CPOR Nodes
**Scenario**: Validate PARTID storage behaviour for CPOR-capable nodes.

**Algorithm**:
- Get the LLC index and cache identifier; skip the test if either is missing or invalid.
- Loop through all MSC nodes and their resources to find PE_CACHE resources matching the LLC identifier that support CPOR; for each valid node:
  - Configure RIS if supported.
  - Update the max cache size.
  - Count CSU monitors if supported.
  - Track the number of CPOR-capable nodes.
- Skip the test if no CPOR nodes or CSU monitors are found.
- Loop through MSC nodes again and for each valid CPOR-capable PE_CACHE resource, configure CPOR partitioning to 75% using the selected test PARTID.
- Create two PARTIDs (partid1 and partid2) for generating PE traffic; read and store the current MPAM2_EL2 value, clear PARTID_D and PMG_D fields, set PARTID to partid1 and PMG to default, and write the updated MPAM2_EL2 value.
- For each matching PE_CACHE resource, allocate source and destination memory buffers sized to 50% of cache size, log buffer size, and skip if allocation fails.
- Configure CSU monitor with partid1, enable monitoring, wait for NRDY timeout, perform a memory transaction, and record updated CSU monitor count as storage_value1.
- Reconfigure CSU monitor with partid2, enable monitoring again, wait for NRDY timeout, perform a second memory transaction, and record updated CSU monitor count as storage_value2.
- Disable the monitor, restore MPAM2_EL2 to its original value, and free the memory buffers.
- Fail the test if storage_value1 is zero (no activity for partid1) or if storage_value2 is non-zero (unexpected activity for partid2); otherwise, mark the test as passed.

---

#### Test 154 Check PARTID Storage by CCAP Nodes
**Scenario**: Validate PARTID storage behaviour for CCAP-capable nodes.

**Algorithm**:
- Get the LLC index and cache identifier; skip the test if either is missing or invalid.
- Loop through all MSC nodes and their resources to find PE_CACHE resources matching the LLC identifier that support CCAP; for each valid node:
  - Configure RIS if supported.
  - Update the max cache size.
  - Count CSU monitors if supported.
  - Track the number of CCAP-capable nodes.
- Skip the test if no CCAP nodes or CSU monitors are found.
- Loop through MSC nodes again and for each valid CCAP-capable PE_CACHE resource, configure CCAP partitioning to 75% using the selected test PARTID.
- Create two PARTIDs (partid1 and partid2) for generating PE traffic; read and store the current MPAM2_EL2 value, clear PARTID_D and PMG_D fields, set PARTID to partid1 and PMG to default, and write the updated MPAM2_EL2 value.
- For each matching PE_CACHE resource, allocate source and destination memory buffers sized to 50% of cache size, log buffer size, and skip if allocation fails.
- Configure CSU monitor with partid1, enable monitoring, wait for NRDY timeout, perform a memory transaction, and record updated CSU monitor count as storage_value1.
- Reconfigure CSU monitor with partid2, enable monitoring again, wait for NRDY timeout, perform a second memory transaction, and record updated CSU monitor count as storage_value2.
- Disable the monitor, restore MPAM2_EL2 to its original value, and free the memory buffers.
- Fail the test if storage_value1 is zero (no activity for partid1) or if storage_value2 is non-zero (unexpected activity for partid2); otherwise, mark the test as passed.

---

#### Test 155 Check MSMON CTL Disable Behavior
**Scenario**: Validate monitor control disable behavior for CSU monitors.

**Algorithm**:
- Identify an MSC that supports monitoring (for example, CSU) by checking that MPAMF_IDR.HAS_MSMON is set.
- Select a monitor instance using MSMON_CFG_MON_SEL.MON_SEL and, if applicable, set MSMON_CFG_MON_SEL.RIS.
- Ensure the monitor is disabled by setting MSMON_CFG_CSU_CTL.EN = 0.
- Record the current value of the monitor’s counter register (MSMON_CSU.VALUE).
- Generate traffic that would normally increment the counter if the monitor were enabled.
- Re-read the counter value and confirm that it remains unchanged.
- Write a new value manually to the counter register (if writable) and confirm that the new value is accepted.
- If the monitor does not count automatically while disabled and accepts software writes, mark the test as passed.
- If the counter increments without being enabled or refuses valid writes, mark the test as failed.

---

### Error Tests

#### Test 201 Check MSC PARTID selection range error
**Scenario**: Validate error reporting for out-of-range PARTID selections.

**Algorithm**:
- Get PE index and total MSC node count.
- Loop through each MSC; skip if MPAMF_ESR is not supported.
- Save current MPAMF_ECR value for later restore. Reset error code; fail test if reset fails.
- Program MPAMCFG_PART_SEL with out-of-range PARTID. Wait briefly for error to propagate.
- Read MPAMF_ESR and check for expected error code. If error code mismatch, log and mark test as failed.
- Restore original MPAMF_ECR value.
- After loop, skip test if no ESR-capable nodes were found; fail if any mismatch; else pass.

---

#### Test 202 Check request PARTID out-of-range error
**Scenario**: Validate error reporting when a request PARTID exceeds the MSC range.

**Algorithm**:
- Get PE index, total MSC count, and save MPAM2_EL2.
- Loop through MSCs; skip if MPAMF_ESR not supported. Save MPAMF_ECR and reset error code; fail if reset fails.
- Program MPAM2_EL2 with PARTID that is beyond the range of MSC's maximum PARTID; if skipped, restore MPAM2_EL2 and continue. Mark test as executed.
- Wait briefly, read MPAMF_ESR, and check for expected error code; fail if mismatched.
- Restore MPAM2_EL2 and MPAMF_ECR. Mark test as skipped, failed, or passed based on results.

---

#### Test 203 Check MSMON config ID out-of-range error
**Scenario**: Validate error reporting for out-of-range monitor config IDs.

**Algorithm**:
- Get PE index and total number of MSC nodes.
- Loop through MSCs; skip if MPAMF_ESR is not supported. Save MPAMF_ECR and reset any existing error code; fail if reset fails.
- Check if the MSC supports monitors and get the total count of CSU and MBWU monitors.
- Skip if no monitors are present in the MSC. Mark that at least one MSC ran the test.
- Generate a monitor config ID out-of-range error using the monitor count. Wait briefly, read MPAMF_ESR, and verify the error code; fail if it doesn’t match expected.
- Restore original MPAMF_ECR value. Mark test as skipped if no eligible MSCs, failed if any mismatch, otherwise passed.

---

#### Test 204 Check request PMG out-of-range error
**Scenario**: Validate error reporting when a request PMG exceeds the MSC range.

**Algorithm**:
- Get PE index and total MSC count; save current MPAM2_EL2.
- Loop through MSCs; skip if MPAMF_ESR not supported. Save MPAMF_ECR and reset error code; fail if reset fails.
- Program MPAM2_EL2 with PMG that is beyond the range of MSC's maximum PMG; if skipped, restore MPAM2_EL2 and continue.
- Mark test as executed; wait, read MPAMF_ESR, and compare with expected error code.
- Fail if error code mismatches; restore MPAM2_EL2 and MPAMF_ECR. Set final status as skipped, failed, or passed based on test outcome.

---

#### Test 205 Check MSC MONITOR selection range error
**Scenario**: Validate error reporting for out-of-range monitor selection.

**Algorithm**:
- Get PE index and total MSC count. Loop through MSCs; skip if ESR not supported or no monitors are implemented.
- Count CSU and MBWU monitors; skip MSC if none found.
- Save MPAMF_ECR and reset error code; fail if reset fails. Mark test as executed, generate monitor selection range error using total monitor count.
- Wait, read MPAMF_ESR, and check for expected error code; fail if mismatched.
- Restore MPAMF_ECR and update final test status as skipped, failed, or passed.

---

#### Test 206 Check intPARTID out-of-range error
**Scenario**: Validate error reporting for out-of-range internal PARTID values.

**Algorithm**:
- Get PE index and total MSC node count. Loop through MSCs; skip if MPAMF_ESR or PARTID narrowing not supported.
- Save MPAMF_ECR and reset error code; fail if reset fails. Mark test as executed, get max PARTID and max internal PARTID.
- Write a valid reqPARTID and map it to an out-of-range intPARTID (max intPARTID + 1).
- Wait, read MPAMF_ESR, and verify expected error code; fail if mismatched.
- Restore MPAMF_ECR and set final test result as skipped, failed, or passed.

---

#### Test 207 Check Unexpected internal error
**Scenario**: Validate error reporting for unexpected internal errors.

**Algorithm**:
- Get PE index and total number of MSCs.
- Loop through MSCs; skip if MPAMF_ESR or PARTID narrowing is not supported. Save MPAMF_ECR and reset error code; fail if reset fails.
- Mark test as executed, get max PARTID and max internal PARTID.
- Configure MPAMCFG_PART_SEL to select internal mapping using a valid PARTID.
- Configure MPAMCFG_INTPARTID with the max internal PARTID to trigger an unexpected internal error.
- Wait, read MPAMF_ESR, and check for expected error code; fail if mismatched. Restore MPAMF_ECR and set final status as skipped, failed, or passed.

---

#### Test 208 Check undefined RIS in MPAMCFG_PART_SEL error
**Scenario**: Validate error reporting for undefined RIS in MPAMCFG_PART_SEL.

**Algorithm**:
- Get PE index and total number of MSCs.
- Loop through MSCs; skip if MPAMF_ESR or RIS is not supported. Save MPAMF_ECR and reset error code; fail if reset fails.
- Get max PARTID and max RIS index, then write an out-of-range RIS to MPAMCFG_PART_SEL.
- Trigger error by accessing the same register; wait, then read MPAMF_ESR.
- If expected RIS error code is not seen, check if MPAMCFG_PART_SEL behaves as RAZ/WI; fail if not.
- Restore MPAMF_ECR, and mark the test as skipped, failed, or passed based on result.

---

#### Test 209 Check RIS no control error
**Scenario**: Validate RIS_NO_CTRL error reporting when control registers are unimplemented.

**Algorithm**:
- Get PE index and total MSC node count. Loop through MSCs; skip if MPAMF_ESR or RIS is not supported.
- For each resource in the MSC, save MPAMF_ECR and reset error code; fail if reset fails. Select the current RIS index using MPAMCFG_PART_SEL.RIS.
- Check if CCAP, CPOR, or MBW partitioning is unimplemented; if so, write to the corresponding partition register to trigger an error.
- Read MPAMF_ESR and verify if the RIS_NO_CTRL error code is reported.
- If error code mismatches, check if the register behaves as RAZ/WI; fail if not.
- Restore MPAMF_ECR and update final test status as skipped, failed, or passed.

---

#### Test 210 Check undefined RIS in MSMON_CFG_MON_SEL error
**Scenario**: Validate error reporting for undefined RIS in MSMON_CFG_MON_SEL.

**Algorithm**:
- Get PE index and total MSC count. Loop through MSCs; skip if MPAMF_ESR, RIS, or MSMON is not supported.
- Save MPAMF_ECR and reset error code; fail if reset fails. Get max RIS index and write out-of-range RIS to MSMON_CFG_MON_SEL.
- Access the same register to trigger the error; wait and read MPAMF_ESR.
- If expected error code is not reported, verify if register behaves as RAZ/WI; fail if not.
- Restore MPAMF_ECR and mark test as skipped, failed, or passed.

---

#### Test 211 Check RIS no monitor error
**Scenario**: Validate RIS_NO_MON error reporting when monitor registers are unimplemented.

**Algorithm**:
- Get PE index and total number of MSCs. Loop through MSCs; skip if MPAMF_ESR or RIS is not supported.
- For each resource in the MSC, save MPAMF_ECR and reset error code; fail if reset fails. Configure MSMON_CFG_MON_SEL.RIS to select current resource.
- If CSU or MBWU monitoring is not implemented, write to the corresponding unimplemented MSMON_CFG register to trigger an error.
- Read MPAMF_ESR and check for RIS_NO_MON error; if missing, check if register behaves as RAZ/WI and fail if not.
- Restore MPAMF_ECR; mark the test as skipped, failed, or passed based on results.

---

#### Test 212 Check MSC Level-Sensitive Error Interrupt Behaviour
**Scenario**: Validate behaviour of level-sensitive MSC error interrupts.

**Algorithm**:
- Get PE index and total MSC count. Loop through MSCs; skip if MPAMF_ESR is not supported.
- Get error interrupt number and type; skip if interrupt is not level-triggered or not implemented.
- Save MPAMF_ECR and reset error code; fail if reset fails. Register the interrupt handler and route the interrupt to the PE.
- Trigger an MSC error interrupt by writing nonzero value to MPAMF_ESR. Poll until interrupt is received or timeout occurs.
- Restore MPAMF_ECR; fail if interrupt not received in time.
- Mark test as skipped if no MSC supports error interrupt; otherwise, pass or fail based on result.

---

#### Test 213 Check MSC Edge-Trigger Error Interrupt Behaviour
**Scenario**: Validate behaviour of edge-triggered MSC error interrupts.

**Algorithm**:
- Get PE index and total MSC count. Loop through MSCs; skip if MPAMF_ESR not supported.
- Get error interrupt number and flags; skip if not edge-triggered or not implemented.
- Save MPAMF_ECR and reset error code; fail if reset fails. Register interrupt handler and route the interrupt to the PE.
- Try to trigger an MSC error interrupt via MPAMF_ESR. Poll for interrupt completion; if handler catches interrupt in time, fail the test.
- Restore MPAMF_ECR and mark test as passed, failed, or skipped based on result.

---

#### Test 214 Check MBWU monitor overflow interrupt functionality
**Scenario**: Validate MBWU monitor overflow interrupt handling.

**Algorithm**:
- Get PE index and total MSC count, save and update MPAM2_EL2 with default PARTID and PMG.
- Loop through MSCs and their resources; check for RIS, MBWU monitor, and overflow interrupt support.
- Skip if monitor or interrupt not supported; otherwise, register interrupt handler and route it to PE. Trigger MBWU overflow by enabling monitor, writing the maximum supported counter value to MBWU monitor and performing large memory copy using 10MB buffers.
- Wait for NRDY timeout, perform memory copy, and poll for interrupt completion.
- If interrupt not received in time, fail the test; otherwise, reset status and free buffers. Restore MPAM2_EL2, disable and reset monitor after test.
- Mark test as skipped if no MSC supports overflow interrupt; else pass or fail based on result.

---

#### Test 215 Check MPAM IDR Zero for Undefined RIS
**Scenario**: Validate MPAM IDR registers read as zero for undefined RIS values.

**Algorithm**:
- Identify all MSC components that implement RIS (MPAMF_IDR.HAS_RIS == 1).
- For each such MSC, determine MPAMF_IDR.RIS_MAX (maximum defined RIS value).
- For testing, set MPAMCFG_PART_SEL.RIS to a value greater than RIS_MAX (an undefined RIS).
- Attempt to read an MPAMF ID register (for example, MPAMF_CCAP_IDR or MPAMF_MBW_IDR).
- Verify the following behavior:
  - All HAS_* fields in the register must read as 0.
  - MPAMF_ESR must not log an error (ERRCODE == 0).
  - No error interrupt should be raised.
- If any HAS_* field is non-zero or an error is logged or interrupt is generated, fail the test.
- Otherwise, mark the test as passed.

---

#### Test 216 Check MPAMF_ESR.RIS field correctness
**Scenario**: Validate MPAMF_ESR.RIS reports the programmed RIS on error.

**Algorithm**:
- Identify MSC components that implement RIS (MPAMF_IDR.HAS_RIS == 1).
- For each MSC, select a valid RIS value and configure it via MPAMCFG_PART_SEL.RIS.
- Intentionally trigger an error involving a MPAMCFG_* register access (for example, access a non-implemented control).
- Read MPAMF_ESR and verify that MPAMF_ESR.RIS == MPAMCFG_PART_SEL.RIS.
- Next, select a valid RIS via MSMON_CFG_MON_SEL.RIS.
- Trigger an error involving a MSMON_* register access (for example, read from an unimplemented monitor index).
- Read MPAMF_ESR again and verify that MPAMF_ESR.RIS == MSMON_CFG_MON_SEL.RIS.
- If any RIS value in MPAMF_ESR does not match the expected source selector, log an error and fail the test.
- If all comparisons match expected RIS values, mark the test as passed.

---

#### Test 217 Check MPAM error overwrite behavior
**Scenario**: Validate MPAM error overwrite behavior when multiple errors occur.

**Algorithm**:
- Identify an MSC that supports error reporting by checking that MPAMF_IDR.HAS_ESR is set, indicating that the MPAMF_ESR register is implemented.
- Trigger an Invalid PARTID error, such as accessing an undefined or misconfigured MPAM register, to force the hardware to write to MPAMF_ESR.
- Immediately read the MPAMF_ESR register and record the current ERRCODE value. It should be Invalid PARTID Error.
- Verify that the OVRWR bit is 0, as this is the first error occurrence and no previous error was overwritten.
- Without clearing the ERRCODE field, trigger an Invalid PMG error.
- Read MPAMF_ESR again and confirm that ERRCODE has updated to the new error code and OVRWR is now set to 1, indicating that the second error overwrote the first.
- Clear the error status by writing 0 to both the ERRCODE and OVRWR fields in MPAMF_ESR.
- Trigger a new MPAM error after clearing.
- Read MPAMF_ESR once more and verify that ERRCODE reflects the new error and OVRWR remains 0, as there was no overwrite condition this time.
- If all expected bit transitions and error logging behaviors occur as described, the test passes.
- If ERRCODE fails to update or OVRWR does not reflect the overwrite state accurately, the test fails.

---

#### Test 218 Check MBWU_L overflow interrupt status
**Scenario**: Validate MBWU_L overflow interrupt status behavior.

**Algorithm**:
- Identify MSCs that support MBWU long counters by confirming MPAMF_MBWUMON_IDR.HAS_LONG is set.
- Select a valid MBWU monitor instance and configure it using MSMON_CFG_MON_SEL.MON_SEL and, if applicable, MSMON_CFG_MON_SEL.RIS.
- Set MSMON_CFG_MBWU_CTL.OFLOW_INTR_L = 1 to enable interrupt generation on overflow.
- Enable the monitor by setting MSMON_CFG_MBWU_CTL.EN = 1.
- If the counter is writable, initialize the counter close to its maximum value to induce overflow quickly.
- Generate memory traffic matching the monitor’s filter criteria to increment the MBWU_L counter and trigger overflow.
- Monitor MSMON_CFG_MBWU_CTL.OFLOW_STATUS_L and confirm it is set to 1 once the counter overflows.
- Confirm that an MPAM overflow interrupt is delivered upon overflow (via IRQ or MSI if configured).
- Clear the overflow status by writing 0 to OFLOW_STATUS_L and disable the monitor.
- If the overflow and corresponding interrupt are observed as expected, mark the test as passed; otherwise, mark it as failed.

---

#### Test 219 Check MBWU Oflow MSI interrupt status
**Scenario**: Validate MBWU overflow MSI interrupt behavior.

**Algorithm**:
- Program MPAM2_EL2 with DEFAULT_PARTID and default PMG; save the original value for restore.
- For each MSC:
  - Skip if no MBWU monitors are present.
  - Read MPAMF_MSMON_IDR and skip if overflow MSI is not supported.
  - Fetch the MSC device ID and ITS ID for MSI routing.
  - Iterate memory resources; if RIS is supported, select the resource instance.
  - Validate the memory range is large enough for two buffers; allocate source and destination buffers.
  - Request OFLOW_MSI assignment and install the GIC ISR once; route the MSI to the current PE.
  - Enable OFLOW_MSI for the MSC.
  - Configure MBWU monitor:
    - Clear any existing overflow status.
    - Set OFLOW_INTR = 1, OFLOW_INTR_L = 0, and enable PARTID/PMG matching.
    - Pre-fill the MBWU counter near overflow and enable the monitor.
    - Generate memory traffic (memcpy) to trigger overflow.
  - Wait for the MSI handler to run and confirm OFLOW_STATUS is observed set in the handler.
  - The handler clears overflow status, disables OFLOW_MSI, disables the monitor, resets MPAM error status, and signals end-of-interrupt.
  - Fail if the MSI times out, the handler is not invoked, or OFLOW_STATUS was not observed set.
- Cleanup per resource: free buffers, disable and reset MBWU monitor, clear overflow status, and disable OFLOW_MSI.
- Restore MPAM2_EL2; pass if at least one MSI path succeeded and no failures occurred, otherwise fail or skip.

---

#### Test 220 Check MPAM error MSI interrupt status
**Scenario**: Validate MPAM error MSI interrupt behavior.

**Algorithm**:
- Identify MSCs that support ESR, RIS, MON, and ERR_MSI.
- Resolve device and ITS IDs, install an MSI ISR, and route the MSI to the current PE.
- Assign ERR_MSI to the MSC and save MPAMF_ECR and MSMON_CFG_MON_SEL for restore.
- Clear existing errors, enable ERR_MSI, and program MSMON_CFG_MON_SEL with an out-of-range RIS index.
- If the MON_SEL update is accepted, access MSMON_CFG_MON_SEL again to trigger the error.
- Wait for the MSI handler to run and validate:
  - The handler is invoked.
  - MPAMF_ESR reports ERRCODE_UNDEF_RIS_MON_SEL.
  - The out-of-range MON_SEL was not accepted by the MSC.
- In the handler, disable ERR_MSI, disable MBWU monitors, and clear the error code.
- Restore MSMON_CFG_MON_SEL and MPAMF_ECR, clear errors, and disable ERR_MSI before moving to the next MSC.
- Pass if at least one MSC generated the MSI and all validations succeeded; otherwise fail or skip.

---

### Memory Bandwidth Tests

#### Test 301 Check MBWPBM Partitioning
**Scenario**: Validate MBWPBM partitioning behaviour using MBWU monitor counts.

**Algorithm**:
- Get PE index and total MSC count; identify memory resources supporting MBWPBM and track minmax PARTID.
- Skip test if no MBWPBM-supported memory nodes exist. Disable CPOR and CCAP partitioning across all MSCs for minmax PARTID.
- Update MPAM2_EL2 with minmax PARTID and default PMG and prepare a counter array to log MBWU activity.
- For each test scenario (for example, 20% and 5% partition): loop through memory MSC nodes, if MBWPBM is supported:
  - Configure MBWPBM for the scenario and disable MBWMIN/MBWMAX if present. Allocate source and destination buffers based on available address range and required size.
  - Skip if SRAT info is missing or memory cannot be allocated. If MBWU monitor is not found, skip the test.
  - Configure and enable the MBWU monitor, then perform memory copy. Record start and end MBWU monitor counts to measure bandwidth usage.
  - Reset and disable MBWU monitor and free allocated memory. Restore original MPAM2_EL2 settings.
- Compare recorded monitor counts across scenarios; if higher bandwidth usage is recorded in more restricted settings, mark test as failed. Otherwise, mark test as passed.

---

#### Test 302 Check MBWMIN Partitioning
**Scenario**: Validate MBWMIN partitioning behaviour using MBWU monitor counts under contention.

**Algorithm**:
- Get PE index and total MSC count; identify memory nodes supporting MBWMIN and find the minimum valid PARTID. Skip the test if no MBWMIN-capable memory nodes exist.
- Set up MPAM2_EL2 with default PMG and minmax PARTID, and configure all cache nodes to disable CPOR, CCAP, MBWPBM, and MBWMAX. Install exception handlers for sync and async faults.
- For each MBWMIN-capable memory node: configure RIS, disable MBWPBM and MBWMAX.
- Allocate shared buffers and determine buffer size. Launch contention by spawning memory-copy threads on secondary PEs.
- Scenario 1: Configure MBWMIN with BW1 percentage and record MBWU monitor count. Wait for all secondary PEs to complete; if timeout, fail the test.
- Scenario 2: Reconfigure MBWMIN with BW2 percentage and repeat measurement. Compare bandwidth usage across scenarios; fail if BW usage increases when a stricter limit is set.
- Restore MPAM2_EL2 and clean up memory allocations. Mark test as passed if all conditions are met.

---

#### Test 303 Check MBWMAX Partitioning
**Scenario**: Validate MBWMAX partitioning behaviour using MBWU monitor counts.

**Algorithm**:
- Get PE index and total MSC count; identify memory resources supporting MBWMAX and track minmax PARTID.
- Skip test if no MBWMAX-supported memory nodes exist. Disable CPOR and CCAP partitioning across all MSCs for minmax PARTID.
- Update MPAM2_EL2 with minmax PARTID and default PMG and prepare a counter array to log MBWU activity.
- For each test scenario (for example, 20% and 5% partition): loop through memory MSC nodes; if MBWMAX is supported:
  - Configure MBWMAX for the scenario and disable MBWMIN/MBWPBM if present.
  - Allocate source and destination buffers based on available memory and required size. Skip if SRAT info is invalid or buffer allocation fails.
  - If MBWU monitor is not found, skip the test. Configure and enable the MBWU monitor, then perform memory copy.
  - Record start and end MBWU monitor counts to measure bandwidth usage. Reset and disable MBWU monitor and free allocated memory. Restore original MPAM2_EL2 settings.
- Compare recorded monitor counts across scenarios; if usage is higher at a more restrictive setting, mark test as failed. Otherwise, mark test as passed.

---

#### Test 304 Check MBWU MSMON CTL Disable behavior
**Scenario**: Validate MSMON control disable behavior for MBWU monitors.

**Algorithm**:
- Identify an MSC that supports monitoring (for example, MBWU) by checking that MPAMF_IDR.HAS_MSMON is set.
- Select a monitor instance using MSMON_CFG_MON_SEL.MON_SEL and, if applicable, set MSMON_CFG_MON_SEL.RIS.
- Ensure the monitor is disabled by setting MSMON_CFG_MBWU_CTL.EN = 0.
- Record the current value of the monitor’s counter register (MSMON_MBWU.VALUE).
- Generate traffic that would normally increment the counter if the monitor were enabled.
- Re-read the counter value and confirm that it remains unchanged.
- Write a new value manually to the counter register (if writable) and confirm that the new value is accepted.
- If the monitor does not count automatically while disabled and accepts software writes, mark the test as passed.
- If the counter increments without being enabled or refuses valid writes, mark the test as failed.

---

#### Test 305 Check MBWU OFLOW_FRZ overflow behavior
**Scenario**: Validate MBWU overflow freeze behavior.

**Algorithm**:
- Identify an MSC with monitoring support and verify the monitor type (MBWU) includes the OFLOW_FRZ in the MSMON_CFG_<type>_CTL register.
- Select a monitor instance using MSMON_CFG_MON_SEL.MON_SEL, and configure MSMON_CFG_MON_SEL.RIS if applicable.
- Set MSMON_CFG_<type>_CTL.OFLOW_INTR = 0 to avoid interrupt interference during testing.
- Enable overflow freeze behavior by setting MSMON_CFG_<type>_CTL.OFLOW_FRZ = 1.
- Enable the monitor (EN = 1) and, if the counter is writable, initialize it near its maximum value to easily induce overflow.
- Generate traffic to trigger an overflow and read the counter value.
- Confirm that after overflow, the counter value remains frozen (unchanged) despite ongoing traffic.
- Reset OFLOW_FRZ = 0 and check OFLOW_STATUS must also be reset. Generate more traffic.
- Verify that the counter resumes incrementing beyond the frozen value.
- If the counter correctly freezes and resumes based on the OFLOW_FRZ bit setting, mark the test as passed.
- If freezing or resumption fails to behave as expected, mark the test as failed.

This validates that OFLOW_FRZ properly controls whether the monitor counter halts or continues after an overflow.

---

#### Test 306 Check MBWU MSMON OFLOW Reset behavior
**Scenario**: Validate MBWU overflow reset behavior.

**Algorithm**:
- Identify an MSC with monitoring support and ensure the monitor type (MBWU) implements the OFLOW_STATUS field in the MSMON_CFG_CTL register.
- Select a monitor instance via MSMON_CFG_MON_SEL.MON_SEL and, if RIS is implemented, set the appropriate RIS using MSMON_CFG_MON_SEL.RIS.
- Enable the monitor by setting EN = 1 in the control register.
- If the monitor counter is writable, preset it to a value near its maximum, such as 0xFFFFFFF0, to easily cause an overflow.
- Generate memory activity that will trigger the counter to overflow.
- After overflow occurs, verify that OFLOW_STATUS is set to 1.
- To clear the overflow state, write a new value directly to the monitor counter register.
- Resume the memory traffic after resetting the overflow condition.
- Monitor the counter to ensure it resumes counting from the cleared or new value.
- If the counter begins counting again correctly after overflow reset, the test is passed.
- If the counter does not resume or remains stuck despite clearing OFLOW_STATUS or updating the counter, the test fails.

---

## Appendix A. Revisions
This appendix describes the technical changes between released issues of this document.

v26.03_MPAM_0.7.0
- Added BETA scenario algorithms.
- Moved from PDF format to Markdown format.
