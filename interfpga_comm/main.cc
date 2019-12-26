#include <random>
#include <mpi.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef CL_HPP_ENABLE_EXCEPTIONS
#  define CL_HPP_ENABLE_EXCEPTIONS
#endif
#ifndef CL_TARGET_OPENCL_VERSION
#  define CL_TARGET_OPENCL_VERSION 200
#endif
#ifndef CL_HPP_TARGET_OPENCL_VERSION
#  define CL_HPP_TARGET_OPENCL_VERSION 200
#endif
#ifndef CL_USE_DEPRECATED_OPENCL_1_2_APIS
#  define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <CL/cl.h>
#include <CL/cl2.hpp>
#include <CL/cl_ext_intelfpga.h>
#pragma GCC diagnostic pop

int main(int argc, char** argv) {

  MPI_Init(&argc, &argv);
  
  int rank = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  fprintf(stderr, "%d/%d\n", rank, size);

  MPI_Status status;

  ///// Create platform //////
  cl::Platform platform;
  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);

  for (auto &p : platforms) {
    auto const name = p.getInfo<CL_PLATFORM_NAME>();
    if (name.find("Intel(R) FPGA SDK for OpenCL(TM)") != std::string::npos) {
      platform = p;
      break;
    }
  }

  if (!platform()) {
    throw cl::Error(CL_DEVICE_NOT_FOUND, "Platform not found");
  }
  if (cl::Platform::setDefault(platform) != platform) {
    throw cl::Error(CL_DEVICE_NOT_FOUND, "Platform not found");
  }

  ///// Check aocx //////
  auto const fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    perror("open");
    throw cl::Error(CL_INVALID_PROGRAM_EXECUTABLE, "open(2)");
  }

  struct stat st;

  if (fstat(fd, &st)) {
    throw cl::Error(CL_INVALID_PROGRAM_EXECUTABLE, "fstat(2)");
  }

  auto data = new char[st.st_size];
  if (read(fd, data, st.st_size) != st.st_size) {
    throw cl::Error(CL_INVALID_PROGRAM_EXECUTABLE, "read(2)");
  }

  ///// Create context //////
  int dev_idx = 0;
  char const* str;
  str = getenv("OMPI_COMM_WORLD_LOCAL_RANK");
  if (str) {
    dev_idx = atoi(str);
  }

  std::vector<cl::Device> devs;
  platform.getDevices(CL_DEVICE_TYPE_ALL, &devs);
  auto const& dev = devs.at(dev_idx);
  auto ctx = cl::Context{dev};

  cl::Context::setDefault(ctx);
  cl::Device::setDefault(dev);

  fprintf(stderr, "**** platform = %s, dev_idx = %d ****\n", platform.getInfo<CL_PLATFORM_NAME>().c_str(), dev_idx);

  ///// Create program /////
  auto dev_cl = dev();
  auto len = static_cast<size_t>(st.st_size);
  auto image = (const unsigned char *)data;
  cl_int error;

  auto prg = clCreateProgramWithBinary(ctx(), 1, &dev_cl, &len, &image, nullptr, &error);
  cl::detail::errHandler(error, "clCreateProgramWithBinary");

  ///// Create command queue /////
  cl::CommandQueue cq0(ctx, dev);
  cl::CommandQueue cq1(ctx, dev);
    
  ///// Create kernel /////
  cl::Kernel k_send(cl::Program(prg, true), "send");
  cl::Kernel k_recv(cl::Program(prg, true), "recv");

  ///// Create kernel functor /////
  cl::KernelFunctor<cl::Buffer, cl_int, cl_int> f_send(k_send);
  cl::KernelFunctor<cl::Buffer, cl_int, cl_int> f_recv(k_recv);
    
  ///// Create buffer (for host and device) /////
  size_t const numdata = 1;
  size_t const BUF_SIZE = sizeof(cl_float16) * numdata;

  // host
  cl_float16 *h_send;
  cl_float16 *h_recv;
  posix_memalign((void**)&h_send, 64, BUF_SIZE);
  posix_memalign((void**)&h_recv, 64, BUF_SIZE);
  memset(h_send, 0x0, BUF_SIZE);
  memset(h_recv, 0x0, BUF_SIZE);

  // device
  cl::Buffer d_send(ctx, CL_MEM_READ_WRITE, BUF_SIZE);
  cl::Buffer d_recv(ctx, CL_MEM_READ_WRITE, BUF_SIZE);

  ///// Set init data /////
  std::mt19937 g(12345);
  std::uniform_real_distribution<float> d;

  for (int i = 0; i < (int)numdata; i++) {
    h_send[i].s[ 0] = d(g);
    h_send[i].s[ 1] = d(g);
    h_send[i].s[ 2] = d(g);
    h_send[i].s[ 3] = d(g);
    h_send[i].s[ 4] = d(g);
    h_send[i].s[ 5] = d(g);
    h_send[i].s[ 6] = d(g);
    h_send[i].s[ 7] = d(g);
    h_send[i].s[ 8] = d(g);
    h_send[i].s[ 9] = d(g);
    h_send[i].s[10] = d(g);
    h_send[i].s[11] = d(g);
    h_send[i].s[12] = d(g);
    h_send[i].s[13] = d(g);
    h_send[i].s[14] = d(g);
    h_send[i].s[15] = 0;  // must be zero
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

  ///// Run inter-FPGA comm /////
  /////////////////////////////////////////////////////////////////////
  fprintf(stderr, "OpenCL kernel activated (from %d)\n", rank);
  switch (rank) {
    case 0:
      fprintf(stderr, "h_send: %f at rank 0 (before)\n", h_send[0].s[0]);
      f_send(cl::EnqueueArgs(cq0, cl::NDRange(1), cl::NDRange(1)), d_send, (int)numdata, rank);
      cq0.finish();
      break;
    case 1:
      fprintf(stderr, "h_recv: %f at rank 1 (before)\n", h_recv[0].s[0]);
      f_recv(cl::EnqueueArgs(cq1, cl::NDRange(1), cl::NDRange(1)), d_recv, (int)numdata, rank);
      cq1.finish();
      break;
  }
  fprintf(stderr, "OpenCL kernel terminated (from %d)\n", rank);
  /////////////////////////////////////////////////////////////////////

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Datatype float16;
  MPI_Type_contiguous(16, MPI_FLOAT, &float16);
  MPI_Type_commit(&float16);

  switch (rank) {
    case 0:
      MPI_Recv(h_recv, numdata, float16, 1, 0, MPI_COMM_WORLD, &status);
      fprintf(stderr, "h_recv: %f at rank 0 (after)\n", h_recv[0].s[0]);
      break;
    case 1:
      cq1.enqueueReadBuffer(d_recv, CL_TRUE, 0, BUF_SIZE, h_recv);
      cq1.finish();
      fprintf(stderr, "h_recv: %f at rank 1 (after)\n", h_recv[0].s[0]);
      MPI_Send(h_recv, numdata, float16, 0, 0, MPI_COMM_WORLD);
      break;
  }

  if (rank == 0) {
    for (int i = 0; i < (int)numdata; i++) {
      if (h_send[i].s[ 0] != h_recv[i].s[ 0]) fprintf(stderr, "ERROR at 0\n");
      if (h_send[i].s[ 1] != h_recv[i].s[ 1]) fprintf(stderr, "ERROR at 1\n");
      if (h_send[i].s[ 2] != h_recv[i].s[ 2]) fprintf(stderr, "ERROR at 2\n");
      if (h_send[i].s[ 3] != h_recv[i].s[ 3]) fprintf(stderr, "ERROR at 3\n");
      if (h_send[i].s[ 4] != h_recv[i].s[ 4]) fprintf(stderr, "ERROR at 4\n");
      if (h_send[i].s[ 5] != h_recv[i].s[ 5]) fprintf(stderr, "ERROR at 5\n");
      if (h_send[i].s[ 6] != h_recv[i].s[ 6]) fprintf(stderr, "ERROR at 6\n");
      if (h_send[i].s[ 7] != h_recv[i].s[ 7]) fprintf(stderr, "ERROR at 7\n");
      if (h_send[i].s[ 8] != h_recv[i].s[ 8]) fprintf(stderr, "ERROR at 8\n");
      if (h_send[i].s[ 9] != h_recv[i].s[ 9]) fprintf(stderr, "ERROR at 9\n");
      if (h_send[i].s[10] != h_recv[i].s[10]) fprintf(stderr, "ERROR at 10\n");
      if (h_send[i].s[11] != h_recv[i].s[11]) fprintf(stderr, "ERROR at 11\n");
      if (h_send[i].s[12] != h_recv[i].s[12]) fprintf(stderr, "ERROR at 12\n");
      if (h_send[i].s[13] != h_recv[i].s[13]) fprintf(stderr, "ERROR at 13\n");
      if (h_send[i].s[14] != h_recv[i].s[14]) fprintf(stderr, "ERROR at 14\n");
      if (h_send[i].s[15] != h_recv[i].s[15]) fprintf(stderr, "ERROR at 15\n");
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();

  return 0;
}
