int read(__global const int *, int, int);
    
__attribute__((reqd_work_group_size(1,1,1)))
__kernel void tb_read(__global int *restrict Y,
                      __global const int *restrict X,
                      __global const int *restrict I,
                      const int N)
{
  /* *Y = read(X, I, VAL); */
  int index, value, cycle;
  for (int i = 0; i < N; i++) {
    index = I[i];
    value = X[index];
    cycle = read(X, index, value);
    Y[i]  = cycle;
  }
}
