# SPDX-License-Identifier: MIT
# Copyright (C) 2026 Advanced Micro Devices, Inc.
#
# Test: col0-23 SHIM MM2S/S2MM (4MB each) + 4-col PLIO (cols 7/11/19/23, 24MB each)
# Source: tarunp/TellurideDDRBwTests ashwinv/plio_plip_wts_bcast_bw_measurement
#   telluride_ddr_bandwidth_suite/tests/T50/plio_plip_tests/shim_to_mem_tests/
#   col0_23_shim_mm2s_s2mm_4MB_each_plus_4plio_24MB_each

set(TEST_NAME "aie2ps_plio_col0_23_shim_mm2s_s2mm_4plio")
set(ELF "${TEST_NAME}.elf")

add_test(NAME "${TEST_NAME}_asm"
  COMMAND aiebu-asm -t aie2ps
    -c "${CMAKE_CURRENT_SOURCE_DIR}/merged_control.asm"
    -o "${ELF}" -f disabledump
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME "${TEST_NAME}_elf_md5sum"
  COMMAND cmake -P "${AIEBU_SOURCE_DIR}/cmake/md5sum-compare.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${ELF}"
    "${CMAKE_CURRENT_SOURCE_DIR}/gold.md5"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set_tests_properties("${TEST_NAME}_asm" PROPERTIES LABELS memcheck)
set_tests_properties("${TEST_NAME}_elf_md5sum" PROPERTIES DEPENDS "${TEST_NAME}_asm")
