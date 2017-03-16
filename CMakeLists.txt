# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: March 2017
cmake_minimum_required(VERSION 2.8.12)
project(MemoryBenchmark CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR})

# Target platform
set(BENCHMARK_TARGET "TUL-KU115" CACHE STRING "Target board for benchmark.")
if (BENCHMARK_TARGET STREQUAL "TUL-KU115")
  set(BENCHMARK_PART_NAME "xcku115-flvb2104-2-e") 
  set(BENCHMARK_DSA_STRING "xilinx:tul-pcie3-ku115:2ddr:3.1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSDACCEL_MEMORYBENCHMARK_TUL_KU115")
  set(BENCHMARK_DIMMS 2)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmark")
elseif (BENCHMARK_TARGET STREQUAL "TUL-KU115-XPR")
  set(BENCHMARK_PART_NAME "xcku115-flvb2104-2-e") 
  set(BENCHMARK_DSA_STRING "xilinx:xil-accel-rd-ku115:4ddr-xpr:3.2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSDACCEL_MEMORYBENCHMARK_TUL_KU115_XPR")
  set(BENCHMARK_DIMMS 4)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmarkFourDimms")
elseif (BENCHMARK_TARGET STREQUAL "ADM-KU3")
  set(BENCHMARK_PART_NAME "xcku060-ffva1156") 
  set(BENCHMARK_DSA_STRING "xilinx:adm-pcie-ku3:2ddr:3.2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSDACCEL_MEMORYBENCHMARK_ADM_KU3")
  set(BENCHMARK_DIMMS 2)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmark")
elseif (BENCHMARK_TARGET STREQUAL "ADM-7V3")
  set(BENCHMARK_PART_NAME "xc7vx690tffg1157-2") 
  set(BENCHMARK_DSA_STRING "xilinx:adm-pcie-7v3:1ddr:3.0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSDACCEL_MEMORYBENCHMARK_ADM_7V3")
  set(BENCHMARK_DIMMS 1)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmark")
else()
  message(FATAL_ERROR "Unknown target \"${BENCHMARK_TARGET}\".")
endif()
set(BENCHMARK_KEEP_INTERMEDIATE OFF CACHE STRING
    "Keep intermediate SDAccel files")

# Domain configuration
set(BENCHMARK_PORT_WIDTH 512 CACHE STRING "Width of port to memory.")
set(BENCHMARK_BURST_COUNT 524288 CACHE STRING "Number of bursts to issue.")
set(BENCHMARK_BURST_LENGTH 2048 CACHE STRING "Length of issued bursts.")
set(BENCHMARK_TARGET_CLOCK 300 CACHE STRING "Target clock speed.")
set(BENCHMARK_HLS_TARGET_TIMING 4 CACHE STRING "Target timing of HLS.")

# Dependencies
find_package(SDAccel REQUIRED)
if (NOT SDACCEL_FOUND)
  message(FATAL_ERROR "SDAccel not found.")
else()
  include_directories(${SDACCEL_INCLUDE_DIR})
endif()
include_directories(${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${SDACCEL_INCLUDE_DIRS} ${SDACCEL_ROOT_DIR}/Vivado_HLS/include)
set(BENCHMARK_LIBS ${SDACCEL_LIBS})

# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Dependencies
set(BENCHMARK_HLS_DEPENDS
    MemoryBenchmark.h.in
    MemoryBenchmark.cpp
    hlslib/OpenCL.h)

# Configure files 
configure_file(MemoryBenchmark.h.in MemoryBenchmark.h)
configure_file(Synthesis.tcl.in Synthesis.tcl)

# Synthesis
add_custom_target(synthesis
  COMMAND ${SDACCEL_VIVADO_HLS} -f Synthesis.tcl
  DEPENDS ${BENCHMARK_HLS_DEPENDS})

# Testing
enable_testing()
add_executable(Testbench Testbench.cpp MemoryBenchmark.cpp)
target_link_libraries(Testbench ${BENCHMARK_LIBS})
add_test(Testbench Testbench)

# SDAccel
add_executable(ExecuteKernel ExecuteKernel.cpp)
target_link_libraries(ExecuteKernel ${BENCHMARK_LIBS})
set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS} 
  # Includes
  -I${CMAKE_BINARY_DIR}
  -I${CMAKE_SOURCE_DIR}
  # Source
  ${CMAKE_SOURCE_DIR}/MemoryBenchmark.cpp
  # Flags
  --kernel ${BENCHMARK_ENTRY_FUNCTION} 
  --xdevice ${BENCHMARK_DSA_STRING}
  --xp prop:kernel.${BENCHMARK_ENTRY_FUNCTION}.kernel_flags="${CMAKE_CXX_FLAGS}"
  --kernel_frequency=${BENCHMARK_TARGET_CLOCK})
if (BENCHMARK_DIMMS GREATER 3)
  set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM2.core.OCL_REGION_0.M02_AXI
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM3.core.OCL_REGION_0.M03_AXI)
endif()
if (BENCHMARK_DIMMS GREATER 1)
  set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM0.core.OCL_REGION_0.M00_AXI
    --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM1.core.OCL_REGION_0.M01_AXI
    --max_memory_ports all)
endif()

add_custom_target(kernel
  COMMAND ${SDACCEL_XOCC} -t hw ${BENCHMARK_XOCC_FLAGS}
  -o memory_benchmark.xclbin
  DEPENDS ${BENCHMARK_HLS_DEPENDS})
