#ifndef INCLUDE_GUARD_CL_UTILS_H
#define INCLUDE_GUARD_CL_UTILS_H
#include <CL/cl.h>

#define MAX_BINARY_SIZE (0x10000000)

extern cl_platform_id platform;
extern cl_command_queue cq;
extern cl_kernel kernel;
extern cl_int ret;
extern cl_mem d_a;
extern cl_mem d_b;
extern cl_mem d_c;
extern cl_uint numdata;

void init_ocl(const char *filename);
void cleanup_ocl();

#endif  // INCLUDE_GUARD_CL_UTILS_H
