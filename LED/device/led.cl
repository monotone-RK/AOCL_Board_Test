#pragma OPENCL EXTENSION cl_intel_channels : enable

channel uint outLED
__attribute__((depth(0))) __attribute__((io("ledcon")));

__kernel void led(int N)
{
  write_channel_intel(outLED, N);
}
