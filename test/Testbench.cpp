/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      May 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License.

#include <iostream>
#include <vector>
#include "MemoryBenchmark.h"

int main(const int argc, char const *const *const argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./Testbench <burst length> <burst count> <gap>\n";
    return 1;
  }

  const unsigned burst_length = std::stoul(argv[1]);
  const unsigned burst_count = std::stoul(argv[2]);
  const unsigned gap = std::stoul(argv[3]);

  std::vector<Data_t> read(kMemorySize, 1);
  std::vector<std::vector<Data_t>> write(9,
                                         std::vector<Data_t>(kMemorySize, 0));
  Data_t read_result[2];
  std::cout << "Read/write two dimms...\n" << std::flush;
  ReadWriteTwoDimms(read.data(), write[0].data(), burst_length, burst_count,
                    gap);
  std::cout << "Read/write four dimms...\n" << std::flush;
  ReadWriteFourDimms(read.data(), write[1].data(), read.data(), write[2].data(),
                     burst_length, burst_count, gap);
  std::cout << "Read two dimms...\n" << std::flush;
  ReadTwoDimms(read.data(), read.data(), &read_result[0], burst_length,
               burst_count, gap);
  std::cout << "Read four dimms...\n" << std::flush;
  ReadFourDimms(read.data(), read.data(), read.data(), read.data(),
                &read_result[1], burst_length, burst_count, gap);
  std::cout << "Write two dimms...\n" << std::flush;
  WriteTwoDimms(write[3].data(), write[4].data(), burst_length, burst_count,
                gap);
  std::cout << "Write four dimms...\n" << std::flush;
  WriteFourDimms(write[5].data(), write[6].data(), write[7].data(),
                 write[8].data(), burst_length, burst_count, gap);
  std::cout << "Verifying...\n" << std::flush;
  for (unsigned i = 0; i < burst_count; ++i) {
    const int begin = i * (burst_length + gap);
    const int end = i * (burst_length + gap) + burst_length;
    if (end >= kMemorySize) {
      break;
    }
    for (int j = 0; j < 9; ++j) {
      for (int k = begin; k < end; ++k) {
        if (write[j][k] != read[k]) {
          std::cerr << "Verification failed." << std::endl;
          return 1;
        }
      }
    }
  }
  std::cout << "Verification ran successfully.\n";

  return 0;
}
