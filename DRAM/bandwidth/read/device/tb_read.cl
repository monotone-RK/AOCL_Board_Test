int read(__global const int *, int);
    
__attribute__((reqd_work_group_size(1,1,1)))
__kernel void tb_read(__global int *restrict Y,
                      __global const int *restrict X,
                      int N)
{
  Y[0] = read(X, N);
}
