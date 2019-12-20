# [rkobayashi@ppxsvc] $ module list
# Currently Loaded Modulefiles:
#   1) quartus/18.1.1.263.p1   2) aocl/520n               3) gcc/4.8.5               4) openmpi/3.1.0

compile:
	mpicxx -fopenmp -O3 -Wall -Wextra -std=c++0x -march=native -g -o interfpga_comm.exe $(shell aocl compile-config) main.cc $(shell aocl link-config)

run:
	salloc -w ppx2-02,ppx2-03 -n 2 -p smi env CL_CONTEXT_COMPILER_MODE_INTELFPGA=3 mpirun interfpga_comm.exe test.aocx

gen:
	srun -u -p syn2 -w ppxsyn03 aoc -board=p520_max_sg280h -fp-relaxed -g -report -v -save-temps test.cl

config:
	srun -u -w ppx2-02 -p smi env ACL_PCIE_USE_JTAG_PROGRAMMING=1 aocl program acl0 test.aocx
	srun -u -w ppx2-03 -p smi env ACL_PCIE_USE_JTAG_PROGRAMMING=1 aocl program acl0 test.aocx

diagnose:
	srun -u -w ppx2-02 -p smi aocl diagnose
	srun -u -w ppx2-03 -p smi aocl diagnose

clean:
	rm -f interfpga_comm.exe
