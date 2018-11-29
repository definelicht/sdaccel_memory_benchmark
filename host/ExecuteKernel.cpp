/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include <iomanip>
#include <iostream>
#include <string>
#include "MemoryBenchmark.h"
#include "hlslib/SDAccel.h"

enum class Mode {
  read,
  write,
  read_write,
};

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./Testbench <mode [read/write/read_write]> <burst "
                 "length> <burst count> <gap>\n";
    return 1;
  }

  Mode mode;
  const std::string mode_arg(argv[1]);
  if (mode_arg == "read") {
    mode = Mode::read;
  } else if (mode_arg == "write") {
    mode = Mode::write;
  } else if (mode_arg == "read_write") {
    mode = Mode::read_write;
  } else {
    std::cerr << "Unrecognized mode: " << mode_arg << "\n";
    return 1;
  }
  const unsigned burst_length = std::stoul(argv[2]);
  const unsigned burst_count = std::stoul(argv[3]);
  const unsigned gap = std::stoul(argv[4]);

  std::cout << "Initializing host memory...\n" << std::flush;
  std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 4096>> read;
  std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 4096>> write;
  if (mode == Mode::read || mode == Mode::read_write) {
    read = decltype(read)(kMemorySize, 1);
  }
  if (mode == Mode::write || mode == Mode::read_write) {
    write = std::vector<Data_t, hlslib::ocl::AlignedAllocator<Data_t, 4096>>(
        kMemorySize, 0);
  }

  try {
    std::cout << "Creating OpenCL context...\n" << std::flush;
    hlslib::ocl::Context context;

    using ReadBuffer_t = hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::read>;
    using WriteBuffer_t =
        hlslib::ocl::Buffer<Data_t, hlslib::ocl::Access::write>;

    std::cout << "Initializing device memory...\n" << std::flush;
    ReadBuffer_t read_device[4];
    WriteBuffer_t write_device[4];
    WriteBuffer_t read_out_device;
    if (mode == Mode::read) {
      read_device[0] = ReadBuffer_t(context, hlslib::ocl::MemoryBank::bank0,
                                    read.cbegin(), read.cend());
      read_device[1] = ReadBuffer_t(context, hlslib::ocl::MemoryBank::bank1,
                                    read.cbegin(), read.cend());
      read_out_device = WriteBuffer_t(context, 1);
      if (kDimms > 2) {
        read_device[2] = ReadBuffer_t(context, hlslib::ocl::MemoryBank::bank2,
                                      read.cbegin(), read.cend());
        read_device[3] = ReadBuffer_t(context, hlslib::ocl::MemoryBank::bank3,
                                      read.cbegin(), read.cend());
      }
    } else if (mode == Mode::write) {
      write_device[0] = WriteBuffer_t(context, hlslib::ocl::MemoryBank::bank0,
                                      write.cbegin(), write.cend());
      write_device[1] = WriteBuffer_t(context, hlslib::ocl::MemoryBank::bank1,
                                      write.cbegin(), write.cend());
      if (kDimms > 2) {
        write_device[2] = WriteBuffer_t(context, hlslib::ocl::MemoryBank::bank2,
                                        write.cbegin(), write.cend());
        write_device[3] = WriteBuffer_t(context, hlslib::ocl::MemoryBank::bank3,
                                        write.cbegin(), write.cend());
      }
    } else if (mode == Mode::read_write) {
      read_device[0] = ReadBuffer_t(context, hlslib::ocl::MemoryBank::bank0,
                                    read.cbegin(), read.cend());
      write_device[0] = WriteBuffer_t(context, hlslib::ocl::MemoryBank::bank1,
                                      write.cbegin(), write.cend());
      if (kDimms > 2) {
        read_device[1] = ReadBuffer_t(context, hlslib::ocl::MemoryBank::bank2,
                                      read.cbegin(), read.cend());
        write_device[1] = WriteBuffer_t(context, hlslib::ocl::MemoryBank::bank3,
                                        write.cbegin(), write.cend());
      }
    }

    std::pair<double, double> elapsed;

    std::cout << "Programming device...\n" << std::flush;
    if (mode == Mode::read) {
      auto program = context.MakeProgram("MemoryBenchmark_Read.xclbin");
      if (kDimms <= 2) {
        auto kernel = program.MakeKernel(
            ReadTwoDimms, "ReadTwoDimms", read_device[0], read_device[1],
            read_out_device, burst_length, burst_count, gap);
        std::cout << "Executing kernel...\n" << std::flush;
        elapsed = kernel.ExecuteTask();
      } else {
        auto kernel =
            program.MakeKernel(ReadFourDimms, "ReadFourDimms", read_device[0],
                               read_device[1], read_device[2], read_device[3],
                               read_out_device, burst_length, burst_count, gap);
        std::cout << "Executing kernel...\n" << std::flush;
        elapsed = kernel.ExecuteTask();
      }
    } else if (mode == Mode::write) {
      auto program = context.MakeProgram("MemoryBenchmark_Write.xclbin");
      if (kDimms <= 2) {
        auto kernel =
            program.MakeKernel(WriteTwoDimms, "WriteTwoDimms", write_device[0],
                               write_device[1], burst_length, burst_count, gap);
        std::cout << "Executing kernel...\n" << std::flush;
        elapsed = kernel.ExecuteTask();
      } else {
        auto kernel = program.MakeKernel(
            WriteFourDimms, "WriteFourDimms", write_device[0], write_device[1],
            write_device[2], write_device[3], burst_length, burst_count, gap);
        std::cout << "Executing kernel...\n" << std::flush;
        elapsed = kernel.ExecuteTask();
      }
    } else {
      auto program = context.MakeProgram("MemoryBenchmark_ReadWrite.xclbin");
      if (kDimms <= 2) {
        auto kernel = program.MakeKernel(ReadWriteTwoDimms, "ReadWriteTwoDimms",
                                         read_device[0], write_device[0],
                                         burst_length, burst_count, gap);
        std::cout << "Executing kernel...\n" << std::flush;
        elapsed = kernel.ExecuteTask();
      } else {
        auto kernel =
            program.MakeKernel(ReadWriteFourDimms, "ReadWriteFourDimms",
                               read_device[0], write_device[0], read_device[1],
                               write_device[1], burst_length, burst_count, gap);
        std::cout << "Executing kernel...\n" << std::flush;
        elapsed = kernel.ExecuteTask();
      }
    }

    unsigned long transferred =
        2 * static_cast<float>((static_cast<long>(burst_count) * burst_length *
                                (kPortWidth / 8)));
    if (kDimms >= 4) {
      transferred *= 2;
    }
    std::cout << "Done.\nTransferred ";
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

  } catch (std::runtime_error const &err) {
    std::cerr << "Execution failed with error: \"" << err.what() << "\"."
              << std::endl;
    return 1;
  }

  return 0;
}
