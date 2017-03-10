#include "MemoryBenchmark.h"
#include <iostream>
#include <vector>

int main() {
  std::vector<Data_t> read(kMemorySize, 1);
  std::vector<Data_t> write(kMemorySize, 0);
  MemoryBenchmark(read.data(), write.data());
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
