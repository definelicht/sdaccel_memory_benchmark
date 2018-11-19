# Author:  Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
# Created: March 2017
# This software is copyrighted under the BSD 3-Clause License. 

cmake_minimum_required(VERSION 3.0)
project(MemoryBenchmark CXX)

# Mandatory parameters
set(BENCHMARK_DSA "" CACHE STRING "The SDAccel DSA (firmware) to target.")
set(BENCHMARK_DIMMS 0 CACHE STRING "How many DDR DIMMs to use for the benchmark.")

# Enforce DSA and DIMMs to be manually specified
if(BENCHMARK_DSA STREQUAL "")
  message(FATAL_ERROR "Please specify the DSA to use by setting -DBENCHMARK_DSA=<value>")
endif()
if(BENCHMARK_DIMMS LESS 1)
  message(FATAL_ERROR "Please specify a positive number of DDR DIMMs to utilize by setting -DBENCHMARK_DIMMS=<value>.")
endif()
if(BENCHMARK_DIMMS EQUAL 1 OR BENCHMARK_DIMMS EQUAL 2)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmark")
elseif(BENCHMARK_DIMMS EQUAL 4)
  set(BENCHMARK_ENTRY_FUNCTION "MemoryBenchmarkFourDimms")
else()
  message(FATAL_ERROR "Unsupported number of DIMMs \"${BENCHMARK_DIMMS}\".")
endif()

# Optional configuration 
set(BENCHMARK_PART_NAME "xcvu9p-flgb2104-2-i") 
set(BENCHMARK_KEEP_INTERMEDIATE OFF CACHE STRING
    "Keep intermediate SDAccel files")
set(BENCHMARK_PORT_WIDTH 512 CACHE STRING "Width of port to memory in bits.")
set(BENCHMARK_MEMORY_SIZE 65536 CACHE STRING "Size of memory in bytes.")
set(BENCHMARK_BUFFER_DEPTH 512 CACHE STRING "Buffer depth between reader and write.")
set(BENCHMARK_TARGET_CLOCK 300 CACHE STRING "Target clock speed.")

# Dependencies
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/hlslib/cmake/FindSDAccel.cmake")
  message(FATAL_ERROR "hlslib not found. Did you remember to checkout submodules? Please run \"git submodule update --init\" now.")
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/hlslib/cmake)
find_package(SDAccel REQUIRED)
find_package(Threads REQUIRED)
include_directories(include hlslib/include ${CMAKE_BINARY_DIR}
                    ${SDAccel_INCLUDE_DIRS})
set(BENCHMARK_LIBS ${SDAccel_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(BENCHMARK_SYNTHESIS_FLAGS "-std=c++11 -DHLSLIB_SYNTHESIS -I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/hlslib/include -I${CMAKE_BINARY_DIR}")
if(((${SDAccel_MAJOR_VERSION} LESS 2018) AND (${SDAccel_MINOR_VERSION} LESS 3)) OR ${SDAccel_MAJOR_VERSION} LESS 2017)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHLSLIB_LEGACY_SDX=1")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHLSLIB_LEGACY_SDX=0")
endif()

# Configure files 
configure_file(include/MemoryBenchmark.h.in MemoryBenchmark.h)
configure_file(script/Synthesis.tcl.in Synthesis.tcl)

# Synthesis
add_custom_target(synthesis
  COMMAND ${SDAccel_VIVADO_HLS} -f Synthesis.tcl)

# Testing
enable_testing()
add_executable(Testbench test/Testbench.cpp kernel/MemoryBenchmark.cpp)
target_link_libraries(Testbench ${BENCHMARK_LIBS})
add_test(Testbench Testbench)

# Hardware kernel 
add_executable(ExecuteKernel.exe host/ExecuteKernel.cpp)
target_link_libraries(ExecuteKernel.exe ${BENCHMARK_LIBS})
set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS} 
  -O3
  # Includes
  -I${CMAKE_BINARY_DIR}
  -I${CMAKE_SOURCE_DIR}/include
  -I${CMAKE_SOURCE_DIR}/hlslib/include
  # Flags
  --kernel ${BENCHMARK_ENTRY_FUNCTION} 
  --xp prop:kernel.${BENCHMARK_ENTRY_FUNCTION}.kernel_flags="${BENCHMARK_SYNTHESIS_FLAGS}"
  --kernel_frequency=${BENCHMARK_TARGET_CLOCK})

# Set first and second DIMM
if(BENCHMARK_DIMMS GREATER 1)
  if(NOT (BENCHMARK_DSA MATCHES "dynamic"))
    set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
      --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM0.core.OCL_REGION_0.M00_AXI
      --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM1.core.OCL_REGION_0.M01_AXI
      --max_memory_ports all)
  else()
    set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
      --sp ${BENCHMARK_ENTRY_FUNCTION}_1.m_axi_gmem0:bank0
      --sp ${BENCHMARK_ENTRY_FUNCTION}_1.m_axi_gmem1:bank1
      --max_memory_ports all)
  endif()
endif()

# Add third and fourth DIMM
if(BENCHMARK_DIMMS GREATER 3)
  if(NOT (BENCHMARK_DSA MATCHES "dynamic"))
    set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
      --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM2.core.OCL_REGION_0.M02_AXI
      --xp misc:map_connect=add.kernel.${BENCHMARK_ENTRY_FUNCTION}_1.M_AXI_GMEM3.core.OCL_REGION_0.M03_AXI)
  else()
    set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS}
      --sp ${BENCHMARK_ENTRY_FUNCTION}_1.m_axi_gmem2:bank2
      --sp ${BENCHMARK_ENTRY_FUNCTION}_1.m_axi_gmem3:bank3)
  endif()
endif()

# Kernel build target
if(((${SDAccel_MAJOR_VERSION} LESS 2018) AND (${SDAccel_MINOR_VERSION} LESS 3)) OR ${SDAccel_MAJOR_VERSION} LESS 2017)
  set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS} --xdevice ${BENCHMARK_DSA})
  add_custom_target(build_kernel
    COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC}
    ${BENCHMARK_XOCC_FLAGS} 
    ${CMAKE_SOURCE_DIR}/kernel/MemoryBenchmark.cpp
    -o MemoryBenchmark.xclbin) 
else()
  set(BENCHMARK_XOCC_FLAGS ${BENCHMARK_XOCC_FLAGS} --platform ${BENCHMARK_DSA})
  add_custom_target(compile_kernel
    COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -c
    ${BENCHMARK_XOCC_FLAGS} ${CMAKE_SOURCE_DIR}/kernel/MemoryBenchmark.cpp
    -o MemoryBenchmark.xo) 
  add_custom_target(link_kernel
    COMMAND XILINX_PATH=${CMAKE_BINARY_DIR} ${SDAccel_XOCC} -l
    ${BENCHMARK_XOCC_FLAGS} 
    MemoryBenchmark.xo -o MemoryBenchmark.xclbin) 
endif()
