#include "hlslib/OpenCL.h"
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
  std::vector<Data_t> write;

  try {

    hlslib::ocl::Context context("Xilinx");

    auto readDevice = context.MakeBuffer<Data_t, hlslib::ocl::Access::read>(
        hlslib::ocl::MemoryBank::bank0, kMemorySize);
#ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115
    auto writeDevice = context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(
        hlslib::ocl::MemoryBank::bank1, kMemorySize);
#else
    auto writeDevice = context.MakeBuffer<Data_t, hlslib::ocl::Access::write>(
        hlslib::ocl::MemoryBank::bank0, kMemorySize);
#endif

    if (verify) {
      read = std::vector<Data_t>(kMemorySize, 1);
      readDevice.CopyToDevice(read.cbegin());
    }

    auto kernel = context.MakeKernelFromBinary(
        "memory_benchmark.xclbin", "MemoryBenchmark", readDevice, writeDevice);

    std::cout << "Executing kernel..." << std::flush;
    const auto elapsed = kernel.ExecuteTask();
    auto transferred =
        2e-9 * static_cast<float>((static_cast<long>(kBurstCount) *
                                   kBurstLength * (kPortWidth / 8)));
    std::cout << " Done.\nTransferred " << std::setprecision(2) << transferred
              << " GB in " << elapsed << " seconds, bandwidth "
              << (transferred / elapsed) << " GB/s" << std::endl;
    if (verify) {
      write.resize(kMemorySize);
      writeDevice.CopyToHost(write.begin());
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
        std::cout << write[j] << " / " << read[j] << "\n";
        if (write[j] != read[j]) {
          std::cerr << "Verification failed." << std::endl;
          return 1;
        }
      }
    }
  }

  return 0;
}
