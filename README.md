About
-----

This small SDAccel project is built to measure the peak memory bandwidth of SDAccel DSAs. It works by reading on one interface into a FIFO buffer, and writing back out to another interface. If four DDR DIMMs are specified, this is done for individually for two pairs.

Building
--------

To build:

```sh
mkdir "<path to build dir>"
cd "<path to build dir>"
cmake "<path to source dir>" -DBENCHMARK_DSA="<DSA string to target, e.g. 'xilinx:tul-pcie3-ku115:2ddr:3.1'>"
make           # Builds host-side software
make synthesis # Runs HLS
make kernel    # Builds hardware kernel
```

Running
-------

Run the binary `ExecuteKernel.exe` built my `make`.
This executable will look for the kernel file `memory_benchmark.xclbin`, which should be located in the same directory. The file will print the result of the benchmark to standard output.

Bugs
----

The code included in this repository has _only_ been tested with SDx 2016.3.
Please report bugs to the issue tracker, or email `johannes.definelicht@inf.ethz.ch`.

