/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include <iomanip>
#include <string>
#include "MemoryBenchmark.h"
#include "hlslib/SDAccel.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: ./Testbench <burst length> <burst count>\n";
    return 1;
  }

  const unsigned burst_length = std::stoul(argv[1]);
  const unsigned burst_count = std::stoul(argv[2]);

  std::cout << "Initializing host memory...\n" << std::flush;
  std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 4096>> read(
      kMemorySize, 1);
  std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 4096>> write0(
      kMemorySize, 0);
  std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 4096>> write1;
  if (kDimms > 2) {
    write1 = decltype(write1)(kMemorySize, 0);
  }

  try {
    std::cout << "Creating OpenCL context...\n" << std::flush;
    hlslib::ocl::Context context;

    std::cout << "Allocating device memory...\n" << std::flush;
    auto read0Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
        hlslib::ocl::MemoryBank::bank0, read.cbegin(), read.cend());
    auto write0Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(
        kDimms == 1 ? hlslib::ocl::MemoryBank::bank0
                    : hlslib::ocl::MemoryBank::bank1,
        write0.cbegin(), write0.cend());
    hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::read> read1Device;
    hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::write> write1Device;
    if (kDimms > 2) {
      read1Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
          hlslib::ocl::MemoryBank::bank2, read.cbegin(), read.cend());
      write1Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(
          hlslib::ocl::MemoryBank::bank3, write1.cbegin(), write1.cend());
    }

    auto program = context.MakeProgram("MemoryBenchmark.xclbin");
    auto kernel =
        kDimms <= 2
            ? program.MakeKernel("MemoryBenchmark", read0Device, write0Device,
                                 burst_length, burst_count)
            : program.MakeKernel("MemoryBenchmarkFourDimms", read0Device,
                                 write0Device, read1Device, write1Device,
                                 burst_length, burst_count);

    std::cout << "Executing kernel..." << std::flush;
    const auto elapsed = kernel.ExecuteTask();
    unsigned long transferred =
        2 * static_cast<float>((static_cast<long>(burst_count) *
                                   burst_length * (kPortWidth / 8)));
    if (kDimms >= 4) {
      transferred *= 2;
    }
    std::cout << " Done.\nTransferred ";
    if (transferred >= 1e9) {
      std::cout << 1e-9 * transferred << " GB";
    } else if (transferred >= 1e6) {
      std::cout << 1e-6 * transferred << " MB";
    } else {
      std::cout << transferred << " B";
    }
    std::cout << " in " << elapsed.second
              << " seconds, corresponding to a bandwidth of "
              << (1e-9 * (transferred / elapsed.second)) << " GB/s."
              << std::endl;
    write0.resize(kMemorySize);
    write0Device.CopyToHost(write0.begin());
    if (kDimms >= 4) {
      write1.resize(kMemorySize);
      write1Device.CopyToHost(write1.begin());
    }

  } catch (std::runtime_error const &err) {
    std::cerr << "Execution failed with error: \"" << err.what() << "\"."
              << std::endl;
    return 1;
  }

  // Verification
  for (unsigned i = 0; i < burst_count; ++i) {
    const unsigned begin = i * (burst_length + 1);
    const unsigned end = i * (burst_length + 1) + burst_length;
    if (end >= kMemorySize) {
      break;
    }
    for (unsigned j = begin; j < end; ++j) {
      if (write0[j] != read[j]) {
        std::cerr << "Verification failed." << std::endl;
        return 1;
      }
      if (kDimms >= 2) {
        if (write1[j] != read[j]) {
          std::cerr << "Verification failed." << std::endl;
          return 1;
        }
      }
    }
  }
  std::cout << "Results successfully verified." << std::endl;

  return 0;
}
