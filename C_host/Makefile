# Compiler
CC := gcc

# Compilation flags
CFLAGS := -O3 -Wall -Wextra -g -std=c99 -fopenmp

# Target
TARGET := test_vecadd.exe
AOCX   := vecadd.aocx

# Files
SRCS := main.c cl_utils.c
LIBS := rt

# Settings of the OpenCL kernel code compilation
CL_KERNEL              := vecadd.cl
OFFLINE_COMPILER       := aoc
OFFLINE_COMPILER_FLAGS := -report -g -v
# if you use a10pl4_dd4gb_gx115_m512, change to -board=a10pl4_dd4gb_gx115_m512
TARGETED_FPGA_BOARD    := p520_max_sg280h

# Make it all!
all: Makefile $(SRCS) $(CL_KERNEL) $(AOCX) compile

compile:
	$(CC) $(CFLAGS) -fPIC $(shell aocl compile-config) $(SRCS) $(shell aocl link-config) $(foreach L,$(LIBS),-l$L) -o $(TARGET)

$(AOCX):
	srun -u -p syn3 -w ppxsyn05 aoc -board=$(TARGETED_FPGA_BOARD) $(OFFLINE_COMPILER_FLAGS) $(CL_KERNEL) -o $(AOCX)

run:
	srun -u -w ppx2-03 -p adm env OMP_NUM_THREADS=14 numactl --cpunodebind=1 --localalloc ./$(TARGET) $(AOCX) 10

emulate:compile
	$(OFFLINE_COMPILER) -march=emulator -legacy-emulator -board=$(TARGETED_FPGA_BOARD) $(OFFLINE_COMPILER_FLAGS) $(CL_KERNEL) -o $(AOCX)
	srun -u -w ppx2-03 -p adm env CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 OMP_NUM_THREADS=14 numactl --cpunodebind=1 --localalloc ./$(TARGET) $(AOCX) 10

config:
	srun -u -p adm -w ppx2-03 env ACL_PCIE_USE_JTAG_PROGRAMMING=1 aocl program acl0 $(AOCX)

.PHONY: clean
clean:
	rm -rf $(TARGET) .emu_models vecadd.aoco vecadd.aocr reprogram_temp.sof
