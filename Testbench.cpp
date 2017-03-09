#include "MemoryBenchmark.h"
#include <iostream>
#include <vector>

int main() {
  std::vector<Data_t> read0(kMemorySize, 1);
  std::vector<Data_t> write0(kMemorySize, 0);
#ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115
  std::vector<Data_t> read1(kMemorySize, 2);
  std::vector<Data_t> write1(kMemorySize, 0);
  MemoryBenchmark(read0.data(), write0.data(), read1.data(), write1.data());
#else
  MemoryBenchmark(read0.data(), write0.data());
#endif
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
