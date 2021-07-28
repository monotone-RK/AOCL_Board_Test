#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cl_utils.h"

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "Usage: ./test_vecadd.exe <AOCX file> <numdata in log scale>\n");
    exit(EXIT_FAILURE);
  }
  if (argc != 3) {
    fprintf(stderr, "Error!\nThe number of arguments is wrong.\n");
    exit(EXIT_FAILURE);
  }

  ///// set numdata and cl env initialized /////
  numdata = (1 << (atoi(argv[2])));
  init_ocl(argv[1]);

  ///// Create host buffer /////
  size_t const BUF_SIZE = sizeof(cl_int) * numdata;
  cl_int *h_a; posix_memalign((void**)&h_a, 64, BUF_SIZE);
  cl_int *h_b; posix_memalign((void**)&h_b, 64, BUF_SIZE);
  cl_int *h_c; posix_memalign((void**)&h_c, 64, BUF_SIZE);

  ///// Set init data /////
#pragma omp parallel for
  for (unsigned int i = 0; i < numdata; i++) {
    h_a[i] = 1;
    h_b[i] = 2;
    h_c[i] = 0;
  }

  ///// main part //////
  fprintf(stderr, "Configuration\n");
  fprintf(stderr, "========================\n");
  fprintf(stderr, "numdata = %u (%zu bytes)\n", numdata, BUF_SIZE);
  fprintf(stderr, "OpenMP Version %d\n", _OPENMP);
  char char_buffer[1024];
  clGetPlatformInfo(platform, CL_PLATFORM_VERSION, 1024, char_buffer, NULL);
  fprintf(stderr, "FPGA programming: %s\n", char_buffer);

  /// Set FPGA data
  ret = clEnqueueWriteBuffer(cq, d_a, CL_TRUE, 0, BUF_SIZE, h_a, 0, NULL, NULL);
  assert(CL_SUCCESS == ret);
  ret = clEnqueueWriteBuffer(cq, d_b, CL_TRUE, 0, BUF_SIZE, h_b, 0, NULL, NULL);
  assert(CL_SUCCESS == ret);

  /// Invoke OpenCL kernel to run vecadd
  size_t gsize[3] = {1, 0, 0};
  size_t lsize[3] = {1, 0, 0};
  ret = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, gsize, lsize, 0, NULL, NULL);
  assert(CL_SUCCESS == ret);
  ret = clFinish(cq);
  assert(CL_SUCCESS == ret);

  /// Do verification
  fprintf(stderr, "\n");
  fprintf(stderr, "Verification\n");
  fprintf(stderr, "========================\n");

  // Retrieve data to be verified from FPGA
  ret = clEnqueueReadBuffer(cq, d_c, CL_TRUE, 0, BUF_SIZE, h_c, 0, NULL, NULL);
  assert(CL_SUCCESS == ret);

  // Check data
#pragma omp parallel for
  for (int i = 0; i < (int)numdata; i++) {
    if (h_c[i] != (h_a[i] + h_b[i])) {
      fprintf(stderr, "Failed!\n");
      fprintf(stderr, "h_c[%d] = %08x, check_data = %08x\n", i, h_c[i], (h_a[i] + h_b[i]));
      exit(EXIT_FAILURE);
    }
  }
  fprintf(stderr, "Passed!\n");

  // Show result
  fprintf(stderr, "------------------------------\n");
  for (int i = 0; i < 10; i++) {
    fprintf(stderr, "h_c[%d] = %08x\n", i, h_c[i]);
  }
  fprintf(stderr, ".....\n");
  for (int i = (int)(numdata-10); i < (int)numdata; i++) {
    fprintf(stderr, "h_c[%d] = %08x\n", i, h_c[i]);
  }
  fprintf(stderr, "------------------------------\n");

  // cleanup
  free(h_a);
  free(h_b);
  free(h_c);
  cleanup_ocl();

  return EXIT_SUCCESS;
}
