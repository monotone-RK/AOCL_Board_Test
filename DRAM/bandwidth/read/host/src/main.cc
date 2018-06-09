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
// This host program executes a simple kernel including an RTL module to evaluate
// bandwidth of memory load access
//
// Verification is performed on the RTL module on FPGA.
///////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdint>
#include <limits>

#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"


// OpenCL runtime configuration
/********************************************************************/
cl_uint                                num_devices   = 0;
cl_context                             context       = NULL;
cl_command_queue                       command_queue = NULL;
cl_program                             program       = NULL;
cl_kernel                              kernel        = NULL;
cl_platform_id                         platform      = NULL;
cl_int                                 status;
cl_event                               write_event[1], kernel_event, finish_event;
cl_mem                                 Y_buf;  // memory object for write
cl_mem                                 X_buf;  // memory object for read
aocl_utils::scoped_array<cl_device_id> device_id;


// Application data on the host PC
/********************************************************************/
std::vector<int>                    cycles_list;  // a list to store the elapsed cycles
aocl_utils::scoped_aligned_ptr<int> X;            // an array to contain integer data sent to the FPGA
size_t                              datanum;      // the number of integer values
size_t                              try_num;      // the number of tries
float                               frequency;    // the operating frequency (assuming MHz)


// variable to activate kernel 
/********************************************************************/
char   *name;
size_t global_item_size[3], local_item_size[3];


// Function prototypes
/********************************************************************/
void init_data();
void init_opencl();
void run();
void readbuf(size_t i);
void verify();
void cleanup();


/********************************************************************/
int main(int argc, char *argv[]) {

  // check command line arguments
  if (argc == 1) { std::cout << "usage: ./host <name> <datanum>"          << std::endl; exit(0); }
  if (argc != 5) { std::cerr << "Error! The number of argument is wrong." << std::endl; exit(1); }
  name      = argv[1];
  datanum   = std::stoull(std::string(argv[2]));
  try_num   = std::stoull(std::string(argv[3]));
  frequency = std::stof(std::string(argv[4]));

  // Initialization
  init_data(); init_opencl(); cycles_list.resize(try_num);

  for (size_t i = 0; i < try_num; ++i) {
    run();      // kernel running
    readbuf(i); // getting the computation results
  }
  
  // verify the computation results
  verify(); 
  
  // Free the resources allocated
  cleanup();
  
  return 0;
}


/********************************************************************/
void init_data() {
  X.reset(datanum);
  #pragma omp parallel for
  for (size_t i = 0; i < datanum; ++i) {
    X[i] = i + 1;
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
  
  std::cout << "Initializing OpenCL" << std::endl;

  if (!aocl_utils::setCwdToExeDir()) exit(1);

  // Get the OpenCL platform.
  platform = aocl_utils::findPlatform("Intel(R) FPGA");  // ~ 16.0: aocl_utils::findPlatform("Altera");
  if (platform == NULL) {
    std::cerr << "ERROR: Unable to find Intel(R) FPGA OpenCL platform." << std::endl;
    exit(1);
  }

  // Query the available OpenCL device.
  device_id.reset(aocl_utils::getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  std::cout << "Platform: " << aocl_utils::getPlatformName(platform).c_str() << std::endl;
  std::cout << "Using " << num_devices << " device(s)" << std::endl;
  std::cout << " " << aocl_utils::getDeviceName(device_id[0]).c_str() << std::endl;
  
  // Create the context.
  context = clCreateContext(NULL, num_devices, device_id, NULL, NULL, &status);
  aocl_utils::checkError(status, "Failed to create context");

  // Create the program for all device. Use the first device as the
  // representative device (assuming all device are of the same type).
  std::string binary_file = aocl_utils::getBoardBinaryFile(name, device_id[0]);
  std::cout << "Using AOCX: " << binary_file.c_str() << std::endl;
  program = createProgramFromBinary(context, binary_file.c_str(), device_id, num_devices);
  
  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  aocl_utils::checkError(status, "Failed to build program");

  // kernel
  kernel = clCreateKernel(program, name, &status);
  if (status != CL_SUCCESS) {
    std::cerr << "clCreateKernel() error" << std::endl;
    exit(1);
  }

  // command queue
  command_queue = clCreateCommandQueue(context, device_id[0], 0, &status);

  // memory object_m
  Y_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_CHANNEL_2_INTELFPGA, sizeof(int), NULL, &status);
  aocl_utils::checkError(status, "Failed to create buffer for Y");
  X_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_CHANNEL_1_INTELFPGA, sizeof(int)*datanum, NULL, &status);
  aocl_utils::checkError(status, "Failed to create buffer for X");

  // host to device_m
  status = clEnqueueWriteBuffer(command_queue, X_buf, CL_TRUE, 0, sizeof(int)*datanum , X , 0, NULL, &write_event[0]);
  aocl_utils::checkError(status, "Failed to transfer input X");

  // Set kernel arguments.
  unsigned argi = 0;
  status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &Y_buf);   aocl_utils::checkError(status, "Failed to set argument Y");
  status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &X_buf);   aocl_utils::checkError(status, "Failed to set argument X");
  status = clSetKernelArg(kernel, argi++, sizeof(int),    &datanum); aocl_utils::checkError(status, "Failed to set argument N");
}


/********************************************************************/
void run() {
  status = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_item_size, local_item_size, 1, write_event, &kernel_event);
  aocl_utils::checkError(status, "Failed to launch kernel");
}


/********************************************************************/
void readbuf(size_t i) {
  // device to host_m
  status = clEnqueueReadBuffer(command_queue, Y_buf, CL_TRUE, 0, sizeof(int), &cycles_list[i], 1, &kernel_event, &finish_event);
  aocl_utils::checkError(status, "Failed to transfer output Y");
}


/********************************************************************/
void verify() {
  bool     error      = false;
  uint64_t cycles_sum = 0;
  std::cout << std::endl;
  #pragma omp parallel for reduction(+:cycles_sum)
  for (size_t i = 0; i < try_num; ++i) {
    // std::cout << "It takes " << cycles_list[i] << " cycles" << std::endl;  // show result
    if (cycles_list[i] == 0) error = true;
    cycles_sum += cycles_list[i];
  }
  if (!error) {
    float avg_cycles   = (float(cycles_sum) / float(try_num));
    float elapsed_time = float(avg_cycles) / (frequency * 1.0e6);
    float bandwidth    = float(sizeof(int) * datanum)/elapsed_time;
    std::cout << "Verification: PASS" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    std::cout << std::setprecision(std::numeric_limits<float>::max_digits10) << "Avg. cycles: " << avg_cycles << std::endl;
    std::cout << "Memory read bandwidth: " << bandwidth * 1.0e-9 << " GB/s (" << elapsed_time << " sec)" << std::endl;
  } else {
    std::cout << "Error! Evaluation failed..." << std::endl;
  }
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
