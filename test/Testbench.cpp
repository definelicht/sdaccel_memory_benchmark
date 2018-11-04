/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      May 2017 
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "MemoryBenchmark.h"
#include <iostream>
#include <vector>

int main() {
  std::vector<Data_t> read(kMemorySize, 1);
  std::vector<Data_t> write0(kMemorySize, 0);
  std::vector<Data_t> write1(kMemorySize, 0);
  std::vector<Data_t> write2(kMemorySize, 0);
  MemoryBenchmark(read.data(), write0.data());
  MemoryBenchmarkFourDimms(read.data(), write1.data(), read.data(),
                           write2.data());
  for (int i = 0; i < kBurstCount; ++i) {
    const int begin = i * (kBurstLength + 1);
    const int end = i * (kBurstLength + 1) + kBurstLength;
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
}
