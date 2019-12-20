#pragma OPENCL EXTENSION cl_intel_channels : enable

/* channel float8 simple_ot __attribute__((depth(0), io("kernel_output_ch0"))); */
/* channel float8 simple_in __attribute__((depth(0), io("kernel_input_ch1"))); */

__kernel void send(__global float8* restrict data, int n, int rank) {
  for (int i = 0; i < n; i++) {
    float8 v = data[i];
    /* write_channel_intel(simple_ot, v); */
    printf("At send (rank: %d)\n", rank);
  }
}

__kernel void recv(__global float8* restrict data, int n, int rank) {
  for (int i = 0; i < n; i++) {
    /* float8 v = read_channel_intel(simple_in); */
    /* data[i] = v; */
    printf("At recv (rank: %d)\n", rank);    
  }
}
