About
-----

This small SDAccel project measures memory bandwidth of SDAccel DSAs. It works by reading from one AXI4 interface into a FIFO buffer, then back to another AXI4 interface. If four DDR DIMMs are specified, this is done for two pairs separately. There is currently no support for running reads-only or writes-only.

Remember to fetch submodules with `git clone --recursive-submodules`, or post-clone with `git submodule update --init`.

Building
--------

To build:

```sh
mkdir "<path to build dir>"
cd "<path to build dir>"
cmake "<path to source dir>" -DBENCHMARK_DSA="<DSA string to target, e.g. 'xilinx_vcu1525_dynamic_5_1'>" -DBENCHMARK_DIMMS="<number of DDR DIMMS to benchmark>"
make                # Builds host-side software
make synthesis      # Runs HLS
make compile_kernel # Compile to .xo container 
make link_kernel    # Link to .xclbin binary 
```

Running
-------

After building with `make`, run the binary `ExecuteKernel.exe` with either `on` or `off` appended for verification.  
This executable will look for the kernel file `MemoryBenchmark.xclbin`, which should be located in the same directory. The file will print the result of the benchmark to standard output.

Adjusting memory access characteristics 
---------------------------------------

The default CMake configuration tests peak memory performance by issuing wide bursts.
In order to test random access, the parameters `BENCHMARK_BURST_LENGTH` (how many elements are requested in a single burst) and `BENCHMARK_BURST_COUNT` (how many bursts of the given length to execute) can be modified, as well as the port width to global memory with `BENCHMARK_PORT_WIDTH`. 

For example, to approximate random access to a 64-bit variable, you could configure the kernel with `-DBENCHMARK_BURST_LENGTH=1 -DBENCHMARK_BURST_COUNT=1073741824 -DBENCHMARK_PORT_WIDTH=64`, which would yield a much lower bandwidth than the default configuration. 

Bugs
----

The code included in this repository has been tested with SDx 2018.2.
Please report bugs using the issue tracker.

