#pragma OPENCL EXTENSION cl_intel_channels : enable

channel float16 simple_ot __attribute__((depth(0), io("tx0")));
channel float16 simple_in __attribute__((depth(0), io("rx1")));

__kernel void send(__global float16* restrict data, int n, int rank) {
  for (int i = 0; i < n; i++) {
    float16 v = data[i];
    write_channel_intel(simple_ot, v);
    /* printf("At send (rank: %d)\n", rank); */
  }
}

__kernel void recv(__global float16* restrict data, int n, int rank) {
  for (int i = 0; i < n; i++) {
    float16 v = read_channel_intel(simple_in);
    data[i] = v;
    /* printf("At recv (rank: %d)\n", rank);     */
  }
}
