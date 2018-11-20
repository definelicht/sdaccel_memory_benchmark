About
-----

This small SDAccel project measures memory bandwidth of SDAccel DSAs. It provides read-only, write-only, and combined read-write kernels that access 1, 2 or 4 DDR banks on the device.

Remember to fetch submodules with `git clone --recursive-submodules`, or post-clone with `git submodule update --init`.

Building
--------

To build (where `mode` can be `[read/write/read_write]`):

```sh
mkdir "<path to build dir>"
cd "<path to build dir>"
cmake "<path to source dir>" -DBENCHMARK_DSA="<DSA string to target, e.g. 'xilinx_vcu1525_dynamic_5_1'>" -DBENCHMARK_DIMMS="<number of DDR DIMMS to benchmark>"
make                  # Builds host-side software
make synthesis_<mode> # Runs HLS (mode can be [read/write/read_write])
make compile_<mode>   # Compile to .xo container
make link_read_<mode> # Link to .xclbin binary 
```

Running
-------

After building with `make`, run the binary `ExecuteKernel.exe <mode> <burst length> <burst count>`.
This executable will look for the kernel file `MemoryBenchmark_<mode>.xclbin`, which should be located in the same directory. The file will print the result of the benchmark to standard output. The Read, Write and ReadWrite kernels must be built separately.

Adjusting memory access characteristics 
---------------------------------------

The default CMake configuration tests peak memory performance by reading 512-bit vectors.
The number of values read in a burst, as well as the total number of bursts to perform, can be specified at runtime. To hide overhead due to executing the OpenCL kernel, use a high burst count.

In order to measure random access, you can modify the `BENCHMARK_MEMORY_PORT_WIDTH` variable to be smaller than 512 bit (this requires re-building the kernel), then use a burst length of 1.

Bugs
----

The code included in this repository has been tested with SDx 2018.2.
Please report bugs using the issue tracker.

