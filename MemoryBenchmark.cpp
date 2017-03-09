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

void MemoryBenchmark(
    Data_t const *in0,
    Data_t *out0
#ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115
    , Data_t const *in1,
    Data_t *out1
#endif
    ) {
  #pragma HLS INTERFACE m_axi port=in0 offset=slave bundle=gmem0 \
    depth=kBurstLength \
    latency=kBurstLength \
    max_read_burst_length=kBurstLength \
    num_read_outstanding=4 \
    max_write_burst_length=kBurstLength \
    num_write_outstanding=4
  #pragma HLS INTERFACE m_axi port=out0 offset=slave bundle=gmem0 \
    depth=kBurstLength \
    latency=kBurstLength \
    max_read_burst_length=kBurstLength \
    num_read_outstanding=4 \
    max_write_burst_length=kBurstLength \
    num_write_outstanding=4
  #pragma HLS INTERFACE s_axilite port=in0    bundle=control 
  #pragma HLS INTERFACE s_axilite port=out0   bundle=control 
  #pragma HLS INTERFACE s_axilite port=return bundle=control 
  #pragma HLS DATAFLOW
#ifdef SDACCEL_MEMORYBENCHMARK_TUL_KU115
  #pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem1 \
    depth=kBurstLength \
    latency=kBurstLength \
    max_read_burst_length=kBurstLength \
    num_read_outstanding=4 \
    max_write_burst_length=kBurstLength \
    num_write_outstanding=4
  #pragma HLS INTERFACE m_axi port=out1 offset=slave bundle=gmem1 \
    depth=kBurstLength \
    latency=kBurstLength \
    max_read_burst_length=kBurstLength \
    num_read_outstanding=4 \
    max_write_burst_length=kBurstLength \
    num_write_outstanding=4
  #pragma HLS INTERFACE s_axilite port=in1  bundle=control 
  #pragma HLS INTERFACE s_axilite port=out1 bundle=control 
  hls::stream<Data_t> buffer0;
  #pragma HLS STREAM variable=buffer0 depth=kBurstLength
  hls::stream<Data_t> buffer1;
  #pragma HLS STREAM variable=buffer1 depth=kBurstLength
  Read(in0, buffer0);
  Read(in1, buffer1);
  Write(buffer0, out0);
  Write(buffer1, out1);
#else
  hls::stream<Data_t> buffer;
  #pragma HLS STREAM variable=buffer depth=kBurstLength
  Read(in0, buffer);
  Write(buffer, out0);
#endif

}
