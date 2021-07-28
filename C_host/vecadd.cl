__kernel void cl_vecadd(
                        __global uint *restrict a,
                        __global uint *restrict b,
                        __global uint *restrict c,
                        const uint numdata
                        )
{
  for (uint i = 0; i < numdata; i++) {
    c[i] = a[i] + b[i];
  }
}
