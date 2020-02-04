#pragma OPENCL EXTENSION cl_intel_channels : enable

channel float8 ch0 __attribute__((depth(16)));

__kernel void send(__global float8* restrict data, int n) {
  for (int i = 0; i < n; i++) {
    float8 v = data[i];
    write_channel_intel(ch0, v);
  }
}

__kernel void recv(__global float8* restrict data, int n) {
  for (int i = 0; i < n; i++) {
    float8 v = read_channel_intel(ch0);
    data[i] = v;
  }
}
