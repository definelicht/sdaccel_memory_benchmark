/// @author    Johannes de Fine Licht (definelicht@inf.ethz.ch)
/// @date      March 2017 
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "MemoryBenchmark.h"
#include "hlslib/Stream.h"
#include "hlslib/Simulation.h"
#include <cassert>

using hlslib::Stream;

void Read(Data_t const *const input, Stream<Data_t, kBufferDepth> &buffer,
          const unsigned burst_length, const unsigned burst_count,
          const unsigned gap) {
ReadCount:
  for (unsigned i = 0; i < burst_count; ++i) {
  ReadBurst:
    for (unsigned j = 0; j < burst_length; ++j) {
      #pragma HLS LOOP_FLATTEN
      #pragma HLS PIPELINE II=1
      const auto index = (i * (burst_length + gap) + j) % kMemorySize;
      assert(index < kMemorySize);
      buffer.Push(input[index]);
    }
  }
}

void Write(Stream<Data_t, kBufferDepth> &buffer, Data_t *const output,
           const unsigned burst_length, const unsigned burst_count,
           const unsigned gap) {
WriteCount:
  for (unsigned i = 0; i < burst_count; ++i) {
  WriteBurst:
    for (unsigned j = 0; j < burst_length; ++j) {
      #pragma HLS LOOP_FLATTEN
      #pragma HLS PIPELINE II=1
      const auto read = buffer.Pop();
      const auto index = (i * (burst_length + gap) + j) % kMemorySize;
      assert(index < kMemorySize);
      output[index] = read;
    }
  }
}

void WriteOnly(Data_t *const output, const unsigned burst_length,
               const unsigned burst_count, const unsigned gap) {
WriteCount:
  for (unsigned i = 0; i < burst_count; ++i) {
  WriteBurst:
    for (unsigned j = 0; j < burst_length; ++j) {
      #pragma HLS LOOP_FLATTEN
      #pragma HLS PIPELINE II=1
      const auto index = (i * (burst_length + gap) + j) % kMemorySize;
      assert(index < kMemorySize);
      output[index] = 1;
    }
  }
}

template <unsigned num_pipes>
void ConsumeReads(Stream<Data_t, kBufferDepth> pipes[], Data_t *out,
                  const unsigned burst_length, const unsigned burst_count) {
ConsumeCount:
  for (unsigned i = 0; i < burst_count; ++i) {
  ConsumeBurst:
    for (unsigned j = 0; j < burst_length; ++j) {
      #pragma HLS LOOP_FLATTEN
      #pragma HLS PIPELINE II=1
      Data_t regs[num_pipes];
    ConsumeUnroll:
      for (unsigned k = 0; k < num_pipes; ++k) {
        #pragma HLS UNROLL
        regs[k] = pipes[k].Pop();
      }
      if (i == burst_count - 1 && j == burst_length - 1) {
        *out = regs[num_pipes - 1];
      }
    }
  }
}

void ReadWriteTwoDimms(Data_t const *in, Data_t *out,
                       const unsigned burst_length,
                       const unsigned burst_count,
                       const unsigned gap) {

  #pragma HLS INTERFACE m_axi port=in  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=in           bundle=control 
  #pragma HLS INTERFACE s_axilite port=out          bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_length  bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_count   bundle=control 
  #pragma HLS INTERFACE s_axilite port=gap           bundle=control 
  #pragma HLS INTERFACE s_axilite port=return       bundle=control 

  #pragma HLS DATAFLOW

  Stream<Data_t, kBufferDepth> buffer("buffer", kBufferDepth);

  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read, in, buffer, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(Write, buffer, out, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FINALIZE()
}

void ReadWriteFourDimms(Data_t const *in0, Data_t *out0,
                        Data_t const *in1, Data_t *out1,
                        const unsigned burst_length,
                        const unsigned burst_count,
                        const unsigned gap) {

  #pragma HLS INTERFACE m_axi port=in0  offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out0 offset=slave bundle=gmem1
  #pragma HLS INTERFACE m_axi port=in1  offset=slave bundle=gmem2
  #pragma HLS INTERFACE m_axi port=out1 offset=slave bundle=gmem3
  #pragma HLS INTERFACE s_axilite port=in0           bundle=control 
  #pragma HLS INTERFACE s_axilite port=out0          bundle=control 
  #pragma HLS INTERFACE s_axilite port=in1           bundle=control 
  #pragma HLS INTERFACE s_axilite port=out1          bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_length  bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_count   bundle=control 
  #pragma HLS INTERFACE s_axilite port=gap           bundle=control 
  #pragma HLS INTERFACE s_axilite port=return        bundle=control 

  #pragma HLS DATAFLOW

  Stream<Data_t, kBufferDepth> buffer0("buffer0");
  Stream<Data_t, kBufferDepth> buffer1("buffer1");

  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read, in0, buffer0, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(Write, buffer0, out0, burst_length, burst_count,
                           gap);
  HLSLIB_DATAFLOW_FUNCTION(Read, in1, buffer1, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(Write, buffer1, out1, burst_length, burst_count,
                           gap);
  HLSLIB_DATAFLOW_FINALIZE()
}

void ReadFourDimms(Data_t const *in0, Data_t const *in1,
                   Data_t const *in2, Data_t const *in3,
                   Data_t *out,
                   const unsigned burst_length,
                   const unsigned burst_count,
                   const unsigned gap) {

  #pragma HLS INTERFACE m_axi port=in0 offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem1
  #pragma HLS INTERFACE m_axi port=in2 offset=slave bundle=gmem2
  #pragma HLS INTERFACE m_axi port=in3 offset=slave bundle=gmem3
  #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem4
  #pragma HLS INTERFACE s_axilite port=in0          bundle=control 
  #pragma HLS INTERFACE s_axilite port=in1          bundle=control 
  #pragma HLS INTERFACE s_axilite port=in2          bundle=control 
  #pragma HLS INTERFACE s_axilite port=in3          bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_length bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_count  bundle=control 
  #pragma HLS INTERFACE s_axilite port=gap          bundle=control 
  #pragma HLS INTERFACE s_axilite port=return       bundle=control 

  #pragma HLS DATAFLOW

  Stream<Data_t, kBufferDepth> pipes[4];

  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read, in0, pipes[0], burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(Read, in1, pipes[1], burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(Read, in2, pipes[2], burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(Read, in3, pipes[3], burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(ConsumeReads<4>, pipes, out, burst_length,
                           burst_count);
  HLSLIB_DATAFLOW_FINALIZE()
}

void ReadTwoDimms(Data_t const *in0, Data_t const *in1,
                  Data_t *out,
                  const unsigned burst_length,
                  const unsigned burst_count,
                  const unsigned gap) {

  #pragma HLS INTERFACE m_axi port=in0 offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem1
  #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem2
  #pragma HLS INTERFACE s_axilite port=in0          bundle=control 
  #pragma HLS INTERFACE s_axilite port=in1          bundle=control 
  #pragma HLS INTERFACE s_axilite port=out          bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_length bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_count  bundle=control 
  #pragma HLS INTERFACE s_axilite port=gap          bundle=control 
  #pragma HLS INTERFACE s_axilite port=return       bundle=control 

  #pragma HLS DATAFLOW

  Stream<Data_t, kBufferDepth> pipes[2];

  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(Read, in0, pipes[0], burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(Read, in1, pipes[1], burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(ConsumeReads<2>, pipes, out, burst_length,
                           burst_count);
  HLSLIB_DATAFLOW_FINALIZE()
}

void WriteFourDimms(Data_t *out0, Data_t *out1,
                    Data_t *out2, Data_t *out3,
                    const unsigned burst_length,
                    const unsigned burst_count,
                    const unsigned gap) {

  #pragma HLS INTERFACE m_axi port=out0 offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out1 offset=slave bundle=gmem1
  #pragma HLS INTERFACE m_axi port=out2 offset=slave bundle=gmem2
  #pragma HLS INTERFACE m_axi port=out3 offset=slave bundle=gmem3
  #pragma HLS INTERFACE s_axilite port=out0          bundle=control 
  #pragma HLS INTERFACE s_axilite port=out1          bundle=control 
  #pragma HLS INTERFACE s_axilite port=out2          bundle=control 
  #pragma HLS INTERFACE s_axilite port=out3          bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_length  bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_count   bundle=control 
  #pragma HLS INTERFACE s_axilite port=return        bundle=control 

  #pragma HLS DATAFLOW

  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(WriteOnly, out0, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(WriteOnly, out1, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(WriteOnly, out2, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(WriteOnly, out3, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FINALIZE()
}

void WriteTwoDimms(Data_t *out0, Data_t *out1,
                   const unsigned burst_length,
                   const unsigned burst_count,
                   const unsigned gap) {

  #pragma HLS INTERFACE m_axi port=out0 offset=slave bundle=gmem0
  #pragma HLS INTERFACE m_axi port=out1 offset=slave bundle=gmem1
  #pragma HLS INTERFACE s_axilite port=out0          bundle=control 
  #pragma HLS INTERFACE s_axilite port=out1          bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_length  bundle=control 
  #pragma HLS INTERFACE s_axilite port=burst_count   bundle=control 
  #pragma HLS INTERFACE s_axilite port=gap           bundle=control 
  #pragma HLS INTERFACE s_axilite port=return        bundle=control 

  #pragma HLS DATAFLOW

  HLSLIB_DATAFLOW_INIT();
  HLSLIB_DATAFLOW_FUNCTION(WriteOnly, out0, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FUNCTION(WriteOnly, out1, burst_length, burst_count, gap);
  HLSLIB_DATAFLOW_FINALIZE()
}
