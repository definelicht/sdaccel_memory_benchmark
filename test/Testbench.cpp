/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017 
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "MemoryBenchmark.h"
#include <iostream>
#include <vector>

int main(const int argc, char const *const *const argv) {

  if (argc != 3) {
    std::cerr << "Usage: ./Testbench <burst length> <burst count>\n";
    return 1;
  }

  const unsigned burst_length = std::stoul(argv[1]);
  const unsigned burst_count = std::stoul(argv[2]);

  std::vector<Data_t> read(kMemorySize, 1);
  std::vector<Data_t> write0(kMemorySize, 0);
  std::vector<Data_t> write1(kMemorySize, 0);
  std::vector<Data_t> write2(kMemorySize, 0);
  MemoryBenchmark(read.data(), write0.data(), burst_length, burst_count);
  MemoryBenchmarkFourDimms(read.data(), write1.data(), read.data(),
                           write2.data(), burst_length, burst_count);
  for (int i = 0; i < burst_count; ++i) {
    const int begin = i * (burst_length + 1);
    const int end = i * (burst_length + 1) + burst_length;
    if (end >= kMemorySize) { 
      break;
    }
    for (int j = begin; j < end; ++j) {
      if (write0[j] != read[j]) {
        std::cerr << "Verification failed." << std::endl;
        return 1;
      }
      if (write1[j] != read[j]) {
        std::cerr << "Verification failed." << std::endl;
        return 1;
      }
      if (write2[j] != read[j]) {
        std::cerr << "Verification failed." << std::endl;
        return 1;
      }
    }
  }

  return 0;
}
