/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017 
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "hlslib/SDAccel.h"
#include "MemoryBenchmark.h"
#include <string>
#include <iomanip>

int main(int argc, char **argv) {

  if (argc > 2) {
    std::cerr << "Usage: ./ExecuteKernel [<verify [on/off]>]" << std::endl;
    return 1;
  }

  bool verify = false;
  if (argc == 2) {
    if (std::string(argv[1]) == "on") {
      verify = true;
    } else if (std::string(argv[1]) == "off") {
      verify = false;
    } else {
      std::cerr << "Verify option must be either \"on\" or \"off\"."
                << std::endl;
      return 1;
    }
  }

  std::vector<Data_t> read;
  std::vector<Data_t> write0;
  std::vector<Data_t> write1;

  try {

    hlslib::ocl::Context context;

    auto read0Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
        hlslib::ocl::MemoryBank::bank0, kMemorySize);
    auto write0Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(
        kDimms == 1 ? hlslib::ocl::MemoryBank::bank0
                    : hlslib::ocl::MemoryBank::bank1,
        kMemorySize);

    hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::read> read1Device;
    hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::write> write1Device;
    if (kDimms >= 2) {
      read1Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
          hlslib::ocl::MemoryBank::bank2, kMemorySize);
      write1Device = context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(
          hlslib::ocl::MemoryBank::bank3, kMemorySize);
    }

    if (verify) {
      read = std::vector<Data_t>(kMemorySize, 1);
      read0Device.CopyFromHost(read.cbegin());
      if (kDimms > 2) {
        read1Device.CopyFromHost(read.cbegin());
      }
    }

    auto kernel =
        kDimms <= 2
            ? context.MakeKernel("memory_benchmark.xclbin", "MemoryBenchmark",
                                 read0Device, write0Device)
            : context.MakeKernel("memory_benchmark.xclbin",
                                 "MemoryBenchmarkFourDimms", read0Device,
                                 write0Device, read1Device, write1Device);

    std::cout << "Executing kernel..." << std::flush;
    const auto elapsed = kernel.ExecuteTask();
    auto transferred =
        2e-9 * static_cast<float>((static_cast<long>(kBurstCount) *
                                   kBurstLength * (kPortWidth / 8)));
    if (kDimms >= 4) {
      transferred *= 2;
    }
    std::cout << " Done.\nTransferred " << std::setprecision(2) << transferred
              << " GB in " << elapsed.first << " seconds, bandwidth "
              << (transferred / elapsed.first) << " GB/s" << std::endl;
    if (verify) {
      write0.resize(kMemorySize);
      write0Device.CopyToHost(write0.begin());
      if (kDimms >= 4) {
        write1.resize(kMemorySize);
        write1Device.CopyToHost(write1.begin());
      }
    }

  } catch (std::runtime_error const &err) {
    std::cerr << "Execution failed with error: \"" << err.what() << "\"."
              << std::endl;
    return 1;
  }

  // Verification
  if (verify) {
    for (int i = 0; i < kBurstCount; ++i) {
      const int begin = i * (kBurstLength + 1);
      const int end = i * (kBurstLength + 1) + kBurstLength;
      if (end >= kMemorySize) { 
        break;
      }
      for (int j = begin; j < end; ++j) {
        std::cout << write0[j] << " / " << read[j] << "\n";
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
  }

  return 0;
}
