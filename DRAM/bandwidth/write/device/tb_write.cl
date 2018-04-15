int write(__global int *, int);

__attribute__((reqd_work_group_size(1,1,1)))
__kernel void tb_write(__global int *restrict Y,
                       __global const int *restrict X,
                       int N)
{
  int tmp;
  tmp = write(Y, N);
}
