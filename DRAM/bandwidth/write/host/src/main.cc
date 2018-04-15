// Copyright (C) 2013-2016 Altera Corporation, San Jose, California, USA. All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to
// whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// This agreement shall be governed in all respects by the laws of the State of California and
// by the laws of the United States of America.

///////////////////////////////////////////////////////////////////////////////////
// This host program executes a simple kernel including an RTL module to perform:
//  Y = X + 1
// where X is array data sent to the FPGA and the computation results are stored
// in Y. 
//
// Verification is performed on the host CPU.
///////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <cstdlib>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

using namespace aocl_utils;

// OpenCL runtime configuration
/********************************************************************/
cl_uint                    num_devices   = 0;
cl_context                 context       = NULL;
cl_command_queue           command_queue = NULL;
cl_program                 program       = NULL;
cl_kernel                  kernel        = NULL;
cl_platform_id             platform      = NULL;
scoped_array<cl_device_id> device_id;
cl_int                     status;
cl_event                   write_event[1], kernel_event, finish_event;
cl_mem                     Y_buf;  // memory object for write
cl_mem                     X_buf;  // memory object for read


// Application data on the host PC
/********************************************************************/
int datanum;                // the number of integer values
scoped_aligned_ptr<int> Y;  // an array to receive the computation results from the FPGA
scoped_aligned_ptr<int> X;  // an array to contain integer data sent to the FPGA


// variable to activate kernel 
/********************************************************************/
char   *name;
size_t global_item_size[3], local_item_size[3];


// Function prototypes
/********************************************************************/
void init_data();
void init_opencl();
void run();
void readbuf();
void verify();
void cleanup();


/********************************************************************/
int main(int argc, char *argv[]) {

  double start, end;
  
  // check command line arguments
  if (argc == 1) { printf("usage: ./host <hogehoge> <datanum>\n");      exit(0); }
  if (argc != 3) { printf("Error! The number of argument is wrong.\n"); exit(1); }
  name    = argv[1];
  datanum = atoi(argv[2]);

  // Initialization
  init_data(); init_opencl();

  // kernel running
  start = getCurrentTimestamp();
  run(); clFinish(command_queue);
  end = getCurrentTimestamp();
  
  // getting the computation results
  readbuf();
  
  // verify the computation results and show the kernel execution time
  verify(); printf("time : %f sec.\n", end-start);
  
  // Free the resources allocated
  cleanup();
  
  return 0;
}


/********************************************************************/
void init_data() {
  Y.reset(datanum);
  X.reset(datanum);
  for (int i = 0; i < datanum; ++i) {
    X[i] = i;
  }
}


/********************************************************************/
void init_opencl() {
  // work item
  local_item_size[2] = 1;
  local_item_size[1] = 1;
  local_item_size[0] = 1;
  global_item_size[2] = 1;
  global_item_size[1] = 1;
  global_item_size[0] = 1;
  
  printf("Initializing OpenCL\n");

  if (!setCwdToExeDir()) exit(1);

  // Get the OpenCL platform.
  // platform = findPlatform("Altera");
  platform = findPlatform("Intel(R) FPGA");
  if (platform == NULL) {
    printf("ERROR: Unable to find Altera OpenCL platform.\n");
    exit(1);
  }

  // Query the available OpenCL device.
  device_id.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  printf("Platform: %s\n", getPlatformName(platform).c_str());
  printf("Using %d device(s)\n", num_devices);
  printf("  %s\n", getDeviceName(device_id[0]).c_str());
  
  // Create the context.
  context = clCreateContext(NULL, num_devices, device_id, NULL, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the program for all device. Use the first device as the
  // representative device (assuming all device are of the same type).
  std::string binary_file = getBoardBinaryFile(name, device_id[0]);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), device_id, num_devices);
  
  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  // kernel
  kernel = clCreateKernel(program, name, &status);
  if (status != CL_SUCCESS) {
    fprintf(stderr, "clCreateKernel() error\n");
    exit(1);
  }

  // command queue
  command_queue = clCreateCommandQueue(context, device_id[0], 0, &status);

  // memory object_m
  Y_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_CHANNEL_1_INTELFPGA, sizeof(int)*datanum, NULL, &status);
  checkError(status, "Failed to create buffer for Y");
  X_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_CHANNEL_2_INTELFPGA, sizeof(int)*datanum, NULL, &status);
  checkError(status, "Failed to create buffer for X");

  // host to device_m
  status = clEnqueueWriteBuffer(command_queue, X_buf, CL_TRUE, 0, sizeof(int)*datanum , X , 0, NULL, &write_event[0]);
  checkError(status, "Failed to transfer input X");

  // Set kernel arguments.
  unsigned argi = 0;
  status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &Y_buf);   checkError(status, "Failed to set argument Y");
  status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &X_buf);   checkError(status, "Failed to set argument X");
  status = clSetKernelArg(kernel, argi++, sizeof(int),    &datanum); checkError(status, "Failed to set argument N");
}


/********************************************************************/
void run() {
  status = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_item_size, local_item_size, 1, write_event, &kernel_event);
  checkError(status, "Failed to launch kernel");
}


/********************************************************************/
void readbuf() {
  // device to host_m
  status = clEnqueueReadBuffer(command_queue, Y_buf, CL_TRUE, 0, sizeof(int)*datanum, Y, 1, &kernel_event, &finish_event);
  checkError(status, "Failed to transfer output Y");
}


/********************************************************************/
void verify() {
  // // show result
  // printf("\n");
  // printf("X[%d] = {", datanum);
  // for (int i = 0; i < datanum; ++i) { printf("%d", X[i]); if (i < datanum - 1) printf(", "); }
  // printf("}\n");
  // printf("Y[%d] = {", datanum);
  // for (int i = 0; i < datanum; ++i) { printf("%d", Y[i]); if (i < datanum - 1) printf(", "); }
  // printf("}\n");

  // verification
  printf("\n");
  bool pass = true;
  for (int i = 0; i < datanum; ++i) {
    if (X[i] != Y[i]) {
      printf("Failed verification!!!\n");
      printf("Y[%d]: %d, expected: %d\n", i, Y[i], X[i]);
      pass = false;
      break;
    }
  }
  printf("Verification: %s\n", pass ? "PASS" : "FAIL");
}


/********************************************************************/
void cleanup() {
  for (int i = 0; i < 1; ++i) clReleaseEvent(write_event[i]);
  clFlush(command_queue);
  clFinish(command_queue);
  clReleaseMemObject(Y_buf);
  clReleaseMemObject(X_buf);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(command_queue);
  clReleaseContext(context);
}
