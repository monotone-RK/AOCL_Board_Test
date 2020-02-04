# [rkobayashi@ppxsvc] $ module list
# Currently Loaded Modulefiles:
#   1) quartus/19.1.0.240   2) aocl/520n_191        3) gcc/4.8.5

compile:
	g++ -fopenmp -O3 -Wall -Wextra -std=gnu++1y -march=native -g -o interkernel_comm.exe $(shell aocl compile-config) -DCL_TARGET_OPENCL_VERSION=200 main.cc $(shell aocl link-config)

emu:
	env CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 ./interkernel_comm.exe test.aocx

egen:
	aoc -march=emulator -emulator-channel-depth-model=default -report -Werror -g -v test.cl

clean:
	rm -rf interkernel_comm.exe test/ test.aoco test.aocr test.aocx
