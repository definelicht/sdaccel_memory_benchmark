/// \author Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// \date March 2017

#include "MemoryBenchmark.h"
#include <hls_stream.h>
#include <cassert>

void Read(Data_t const *input, hls::stream<Data_t> &buffer) {
ReadCount:
  for (int i = 0; i < kBurstCount; ++i) {
  ReadBurst:
    for (int j = 0; j < kBurstLength; ++j) {
      #pragma HLS LOOP_FLATTEN
      #pragma HLS PIPELINE
      const auto index = (i * (kBurstLength + 1) + j) % kMemorySize;
      assert(index >= 0);
      assert(index < kMemorySize);
      buffer.write(input[index]);
    }
  }
}

void Write(hls::stream<Data_t> &buffer, Data_t *output) {
WriteCount:
  for (int i = 0; i < kBurstCount; ++i) {
  WriteBurst:
    for (int j = 0; j < kBurstLength; ++j) {
      #pragma HLS LOOP_FLATTEN
      #pragma HLS PIPELINE
      const auto read = buffer.read();
      const auto index = (i * (kBurstLength + 1) + j) % kMemorySize;
      assert(index >= 0);
      assert(index < kMemorySize);
      output[index] = read;
    }
  }
}

void MemoryBenchmark(Data_t const *in, Data_t *out) {
  #pragma HLS INTERFACE m_axi port=in offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=in     bundle=control 
  #pragma HLS INTERFACE s_axilite port=out    bundle=control 
  #pragma HLS INTERFACE s_axilite port=return bundle=control 
  #pragma HLS DATAFLOW
  hls::stream<Data_t> buffer;
  #pragma HLS STREAM variable=buffer depth=256
  Read(in, buffer);
  Write(buffer, out);
}

void MemoryBenchmarkFourDimms(Data_t const *in0, Data_t *out0,
                              Data_t const *in1, Data_t *out1) {
  #pragma HLS INTERFACE m_axi port=in0  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out1 offset=slave bundle=gmem1
  #pragma HLS INTERFACE m_axi port=in0  offset=slave bundle=gmem2
  #pragma HLS INTERFACE m_axi port=out1 offset=slave bundle=gmem3
  #pragma HLS INTERFACE s_axilite port=in0    bundle=control 
  #pragma HLS INTERFACE s_axilite port=out0   bundle=control 
  #pragma HLS INTERFACE s_axilite port=in1    bundle=control 
  #pragma HLS INTERFACE s_axilite port=out1   bundle=control 
  #pragma HLS INTERFACE s_axilite port=return bundle=control 
  #pragma HLS DATAFLOW
  hls::stream<Data_t> buffer0;
  #pragma HLS STREAM variable=buffer0 depth=256
  hls::stream<Data_t> buffer1;
  #pragma HLS STREAM variable=buffer1 depth=256
  Read(in0, buffer0);
  Write(buffer0, out0);
  Read(in1, buffer1);
  Write(buffer1, out1);
}
