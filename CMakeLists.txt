# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: March 2017
# This software is copyrighted under the BSD 3-Clause License. 

cmake_minimum_required(VERSION 2.8.12)
project(MemoryBenchmark CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Mandatory parameters
set(BENCHMARK_DSA "" CACHE STRING "The SDAccel DSA (firmware) to target.")
set(BENCHMARK_DIMMS 0 CACHE STRING "How many DDR DIMMs to use for the benchmark.")

if(BENCHMARK_DSA STREQUAL "")
  message(FATAL_ERROR "Please specify the DSA to use by setting -DBENCHMARK_DSA=<value>")
endif()
if(BENCHMARK_DIMMS LESS 1)
  message(FATAL_ERROR "Please specify a positive number of DDR DIMMs to utilize.")
endif()
if(BENCHMARK_DIMMS EQUAL 1 OR BENCHMARK_DIMMS EQUAL 2)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmark")
elseif(BENCHMARK_DIMMS EQUAL 4)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmarkFourDimms")
else()
  message(FATAL_ERROR "Unsupported number of DIMMs \"${BENCHMARK_DIMMS}\".")
endif()

# Optional configuration 
set(BENCHMARK_PART_NAME "xcku115-flvb2104-2-e") 
set(BENCHMARK_KEEP_INTERMEDIATE OFF CACHE STRING
    "Keep intermediate SDAccel files")

# Domain configuration
set(BENCHMARK_PORT_WIDTH 512 CACHE STRING "Width of port to memory.")
set(BENCHMARK_BURST_COUNT 524288 CACHE STRING "Number of bursts to issue.")
set(BENCHMARK_BURST_LENGTH 2048 CACHE STRING "Length of issued bursts.")
set(BENCHMARK_TARGET_CLOCK 300 CACHE STRING "Target clock speed.")
set(BENCHMARK_HLS_TARGET_TIMING 3.3333 CACHE STRING "Target timing of HLS.")

# Dependencies
find_package(SDAccel REQUIRED)
include_directories(${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${SDAccel_INCLUDE_DIRS} ${SDAccel_ROOT_DIR}/Vivado_HLS/include)
set(BENCHMARK_LIBS ${SDAccel_LIBRARIES})

# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Dependencies
set(BENCHMARK_HLS_DEPENDS
    MemoryBenchmark.h.in
    MemoryBenchmark.cpp
    hlslib/SDAccel.h)

# Configure files 
configure_file(MemoryBenchmark.h.in MemoryBenchmark.h)
configure_file(Synthesis.tcl.in Synthesis.tcl)

# Synthesis
add_custom_target(synthesis
  COMMAND ${SDAccel_VIVADO_HLS} -f Synthesis.tcl
  DEPENDS ${BENCHMARK_HLS_DEPENDS})

# Testing
enable_testing()
add_executable(Testbench Testbench.cpp MemoryBenchmark.cpp)
target_link_libraries(Testbench ${BENCHMARK_LIBS})
add_test(Testbench Testbench)

# Hardware kernel 
add_executable(ExecuteKernel.exe ExecuteKernel.cpp)
target_link_libraries(ExecuteKernel.exe ${BENCHMARK_LIBS})
set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS} 
  -O3
  # Includes
  -I${CMAKE_BINARY_DIR}
  -I${CMAKE_SOURCE_DIR}
  # Source
  ${CMAKE_SOURCE_DIR}/MemoryBenchmark.cpp
  # Flags
  --kernel ${BENCHMARK_ENTRY_FUNCTION} 
  --xdevice ${BENCHMARK_DSA}
  --xp prop:kernel.${BENCHMARK_ENTRY_FUNCTION}.kernel_flags="${CMAKE_CXX_FLAGS}"
  --kernel_frequency=${BENCHMARK_TARGET_CLOCK})
if(BENCHMARK_DIMMS GREATER 3)
  set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM2.core.OCL_REGION_0.M02_AXI
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM3.core.OCL_REGION_0.M03_AXI)
endif()
if(BENCHMARK_DIMMS GREATER 1)
  set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM0.core.OCL_REGION_0.M00_AXI
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM1.core.OCL_REGION_0.M01_AXI
    --max_memory_ports all)
endif()
add_custom_target(kernel
  COMMAND ${SDAccel_XOCC} -t hw ${BENCHMARK_XOCC_FLAGS}
  -o memory_benchmark.xclbin
  DEPENDS ${BENCHMARK_HLS_DEPENDS})
