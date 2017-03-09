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

  std::vector<Data_t> read0(kBurstCount * kBurstLength, 1);
  std::vector<Data_t> write0(kBurstCount * kBurstLength, 0);
#ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115 
  std::vector<Data_t> read1(kBurstCount * kBurstLength, 2);
  std::vector<Data_t> write1(kBurstCount * kBurstLength, 0);
#endif

  try {

    hlslib::ocl::Context context("Xilinx");

    auto read0Device =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
            hlslib::ocl::MemoryBank::bank0, read0.cbegin(), read0.cend());
    auto write0Device =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
            hlslib::ocl::MemoryBank::bank0, write0.cbegin(), write0.cend());
#ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115
    auto read1Device =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
            hlslib::ocl::MemoryBank::bank1, read1.cbegin(), read1.cend());
    auto write1Device =
        context.MakeBuffer<Data_t, hlslib::ocl::Access::readWrite>(
            hlslib::ocl::MemoryBank::bank1, write1.cbegin(), write1.cend());
#endif

#ifdef SDACCEL_MEMORYBENCHMARK_ADM_7V3
    auto kernel = context.MakeKernelFromBinary(
        "adm-7v3.xclbin", "MemoryBenchmark", read0Device, write0Device);
#else
    auto kernel = context.MakeKernelFromBinary(
        "tul-ku115.xclbin", "MemoryBenchmark", read0Device, write0Device,
        read1Device, write1Device);
#endif

    std::cout << "Executing kernel..." << std::flush;
    const auto elapsed = kernel.ExecuteTask();
    const auto transferred =
        1e-6 * static_cast<float>((static_cast<long>(kBurstCount) *
                                   kBurstLength * (kPortWidth / 8)));
    std::cout << " Done.\nTransferred " << std::setprecision(2) << transferred
              << "MB in " << elapsed << " seconds, bandwidth "
              << (transferred / elapsed) << " MB/s" << std::endl;
    write0Device.CopyToHost(write0.begin());
#ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115
    write1Device.CopyToHost(write1.begin());
#endif

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
        std::cout << write0[j] << " / " << read0[j] << "\n";
        if (write0[j] != read0[j]) {
          std::cerr << "Verification failed." << std::endl;
          return 1;
        }
  #ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115
        if (write1[j] != read1[j]) {
          std::cerr << "Verification failed." << std::endl;
          return 1;
        }
  #endif
      }
    }
  }

  return 0;
}
