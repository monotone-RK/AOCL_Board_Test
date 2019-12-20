#include <stdint.h>
#include <random>
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <math.h>

#define CALL(cq, k, ...) k.bind(cq, cl::NDRange(1), cl::NDRange(1))(__VA_ARGS__)

int main(int argc, char** argv) {

  MPI_Init(&argc, &argv);
  
  int rank = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  fprintf(stderr, "%d/%d\n", rank, size);

  ///// Create platform //////
  std::vector<cl::Platform> ps;
  cl::Platform::get(&ps);
  cl::Platform platform = ps.at(0);

  ///// Create context //////
  std::vector<cl::Device> dev;
  platform.getDevices(CL_DEVICE_TYPE_ALL, &dev);
  cl::Context ctx = cl::Context(dev);
  
  ///// Check aocx //////
  int const fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(1);
  }

  struct stat st;
  if (fstat(fd, &st)) {
    perror("stat");
    exit(1);
  }

  void* aocx = calloc(1, st.st_size);
  ssize_t nr = read(fd, aocx, st.st_size);

  if (nr < 0 || nr != st.st_size) {
    perror("read");
    exit(1);
  }
  
  ///// Create program /////
  cl::Program::Binaries bins{{ aocx, st.st_size }};
  cl::Program prog(ctx, dev, bins);
  
  ///// Create command queue /////
  cl::CommandQueue cq0(ctx, dev.at(0));
  cl::CommandQueue cq1(ctx, dev.at(0));
    
  ///// Create kernel /////
  cl::Kernel send(prog, "send");
  cl::Kernel recv(prog, "recv");
    
  cl_float8 *h_send;
  cl_float8 *h_recv;
  size_t const numdata = 1;
  size_t const BUF_SIZE = sizeof(cl_float8) * numdata;

  posix_memalign((void**)&h_send, 64, BUF_SIZE);
  posix_memalign((void**)&h_recv, 64, BUF_SIZE);

  ///// Create buffer /////
  cl::Buffer d_send(ctx, CL_MEM_READ_WRITE, BUF_SIZE);
  cl::Buffer d_recv(ctx, CL_MEM_READ_WRITE, BUF_SIZE);
  
  memset(h_send, 0x0, BUF_SIZE);
  memset(h_recv, 0x0, BUF_SIZE);

  std::mt19937 g(12345);
  std::uniform_real_distribution<float> d;

  for (int i = 0; i < (int)numdata; i++) {
    h_send[i].s[0] = d(g);
    h_send[i].s[1] = d(g);
    h_send[i].s[2] = d(g);
    h_send[i].s[3] = d(g);
    h_send[i].s[4] = d(g);
    h_send[i].s[5] = d(g);
    h_send[i].s[6] = d(g);
    h_send[i].s[7] = d(g);
  }
  
  switch (rank) {
    case 0:
      cq0.enqueueWriteBuffer(d_send, CL_TRUE, 0, BUF_SIZE, h_send);
      cq0.finish();
      break;
    case 1:
      cq1.enqueueWriteBuffer(d_recv, CL_TRUE, 0, BUF_SIZE, h_recv);
      cq1.finish();
      break;
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // /////////////////////////////////////////////////////////////////////
  fprintf(stderr, "OpenCL kernel activated (from %d)\n", rank);
  switch (rank) {
    case 0:
      CALL(cq0, send, d_send, (int)numdata, rank);
      cq0.finish();
      break;
    case 1:
      CALL(cq1, recv, d_recv, (int)numdata, rank);
      cq1.finish();
      break;
  }
  fprintf(stderr, "OpenCL kernel terminated (from %d)\n", rank);
  // /////////////////////////////////////////////////////////////////////  

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();

  return 0;
}
