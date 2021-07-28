#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cl_utils.h"

cl_platform_id platform = NULL;
cl_context ctx = NULL;
cl_program prg = NULL;
cl_command_queue cq = NULL;
cl_kernel kernel = NULL;
cl_int ret = 0;
cl_mem d_a = NULL;
cl_mem d_b = NULL;
cl_mem d_c = NULL;
cl_uint numdata = 0;

void init_ocl(const char *filename) {
  ///// Create platform //////
  ret = clGetPlatformIDs(1, &platform, NULL);
  assert(CL_SUCCESS == ret);

  ///// Create context //////
  cl_device_id dev = NULL;
  cl_uint num_devs;
  ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &dev, &num_devs);
  assert(CL_SUCCESS == ret);
  ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &ret);
  assert(CL_SUCCESS == ret);

  ///// Create program /////
  // const char *filename = "./vecadd.aocx";
  FILE *fp = fopen(filename, "rb");
  assert((fp != NULL));
  char *binary_buf = (char*)malloc(MAX_BINARY_SIZE);
  size_t binary_size = fread(binary_buf, 1, MAX_BINARY_SIZE, fp);
  fclose(fp);
  prg = clCreateProgramWithBinary(ctx, num_devs, &dev, (const size_t *)&binary_size,
                                  (const unsigned char **)&binary_buf, NULL, &ret);
  assert(CL_SUCCESS == ret);

  ///// Create buffer /////
  size_t const BUF_SIZE = sizeof(cl_int) * numdata;
  d_a = clCreateBuffer(ctx, CL_MEM_READ_WRITE, BUF_SIZE, NULL, &ret);
  assert(CL_SUCCESS == ret);
  d_b = clCreateBuffer(ctx, CL_MEM_READ_WRITE, BUF_SIZE, NULL, &ret);
  assert(CL_SUCCESS == ret);
  d_c = clCreateBuffer(ctx, CL_MEM_READ_WRITE, BUF_SIZE, NULL, &ret);
  assert(CL_SUCCESS == ret);

  ///// Create kernel /////
  kernel = clCreateKernel(prg, "cl_vecadd", &ret);
  assert(CL_SUCCESS == ret);

  ///// Set Kernel Args /////
  unsigned int argi = 0;
  ret = clSetKernelArg(kernel, argi++, sizeof(cl_mem), (void*)&d_a);
  assert(CL_SUCCESS == ret);
  ret = clSetKernelArg(kernel, argi++, sizeof(cl_mem), (void*)&d_b);
  assert(CL_SUCCESS == ret);
  ret = clSetKernelArg(kernel, argi++, sizeof(cl_mem), (void*)&d_c);
  assert(CL_SUCCESS == ret);
  ret = clSetKernelArg(kernel, argi++, sizeof(cl_uint), (void*)&numdata);
  assert(CL_SUCCESS == ret);

  ///// Create command queue /////
  cq = clCreateCommandQueue(ctx, dev, 0, &ret);
  assert(CL_SUCCESS == ret);
}

void cleanup_ocl() {
  // Delete command queue
  clReleaseCommandQueue(cq);

  // Delete kernel
  clReleaseKernel(kernel);

  // Delete buffer
  clReleaseMemObject(d_a);
  clReleaseMemObject(d_b);
  clReleaseMemObject(d_c);

  // Delete program
  clReleaseProgram(prg);

  // Delete context
  clReleaseContext(ctx);
}
