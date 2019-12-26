# [rkobayashi@ppxsvc] $ module list
# Currently Loaded Modulefiles:
#   1) quartus/19.1.0.240   2) aocl/520n_191        3) gcc/4.8.5            4) openmpi/3.1.0

compile:
	mpic++ -fopenmp -O3 -Wall -Wextra -std=gnu++1y -march=native -g -o interfpga_comm.exe $(shell aocl compile-config) -DCL_TARGET_OPENCL_VERSION=200 main.cc $(shell aocl link-config)

run:
	salloc -w ppx2-02,ppx2-03 -n 2 -p smi env CL_CONTEXT_COMPILER_MODE_INTELFPGA=3 mpirun interfpga_comm.exe test.aocx

gen:
	srun -u -p syn2 -w ppxsyn03 aoc -board-package=/path/to/custom_bsp -board=p520_max_sg280h -fp-relaxed -g -report -v -save-temps test.cl

program:
	srun -u -w ppx2-02 -p smi nios2-configure-sof test/top.sof
	srun -u -w ppx2-03 -p smi nios2-configure-sof test/top.sof

config:
	srun -u -w ppx2-02 -p smi aocl program acl0 test.aocx
	srun -u -w ppx2-03 -p smi aocl program acl0 test.aocx

diagnose:
	srun -u -w ppx2-02 -p smi aocl diagnose
	srun -u -w ppx2-03 -p smi aocl diagnose

clean:
	rm -f interfpga_comm.exe
