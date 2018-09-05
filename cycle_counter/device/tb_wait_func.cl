#pragma OPENCL EXTENSION cl_intel_channels : enable

channel long start __attribute__((depth(0))) __attribute__((io("cnt_start")));
channel long stop __attribute__((depth(0))) __attribute__((io("cnt_stop")));
channel long get_cycles __attribute__((depth(0))) __attribute__((io("cnt_rslt")));

long wait_func(const long);
    
__attribute__((reqd_work_group_size(1,1,1)))
__kernel void tb_wait_func(__global long *restrict expected,
                           __global long *restrict measured,
                           const long N)
{
  long __attribute__((register)) cycles_of_wait_func;
  long __attribute__((register)) cycles_of_cycle_counter;

  /* write_channel_intel(start, 0); */
  /* cycles_of_wait_func = wait_func(N); */
  /* write_channel_intel(stop, cycles_of_wait_func); */

  /* cycles_of_cycle_counter = read_channel_intel(get_cycles); */

  cycles_of_wait_func = wait_func(N);

  write_channel_intel(start, 0);
  write_channel_intel(stop, wait_func(N));
  cycles_of_cycle_counter = read_channel_intel(get_cycles);
  
  *expected = cycles_of_wait_func;
  *measured = cycles_of_cycle_counter;
}
