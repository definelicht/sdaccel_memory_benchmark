About
-----

This small SDAccel project is built to measure the peak memory bandwidth of SDAccel DSAs. It works by reading on one interface into a FIFO buffer, and writing back out to another interface. If four DDR DIMMs are specified, this is done for individually for two pairs.

Building
--------

To build:

```sh
mkdir "<path to build dir>"
cd "<path to build dir>"
cmake "<path to source dir>" -DBENCHMARK_DSA="<DSA string to target, e.g. 'xilinx:tul-pcie3-ku115:2ddr:3.1'>" -DBENCHMARK_DIMMS="<number of DDR DIMMS to benchmark>"
make           # Builds host-side software
make synthesis # Runs HLS
make kernel    # Builds hardware kernel
```

Running
-------

After building with `make`, run the binary `ExecuteKernel.exe` with either `on` or `off` appended for verification.  
This executable will look for the kernel file `memory_benchmark.xclbin`, which should be located in the same directory. The file will print the result of the benchmark to standard output.

Adjusting memory access characteristics 
---------------------------------------

The default CMake configuration tests peak memory performance by issuing wide bursts.
In order to test random access, the parameters `BENCHMARK_BURST_LENGTH` (how many elements are requested in a single burst) and `BENCHMARK_BURST_COUNT` (how many bursts of the given length to execute) can be modified, as well as the port width to global memory with `BENCHMARK_PORT_WIDTH`. 

For example, to approximate random access to a 64-bit variable, you could configure the kernel with `-DBENCHMARK_BURST_LENGTH=1 -DBENCHMARK_BURST_COUNT=1073741824 -DBENCHMARK_PORT_WIDTH=64`, which would yield a much lower bandwidth than the default configuration. 

Bugs
----

The code included in this repository has _only_ been tested with SDx 2016.3.
Please report bugs to the issue tracker, or email `johannes.definelicht@inf.ethz.ch`.

