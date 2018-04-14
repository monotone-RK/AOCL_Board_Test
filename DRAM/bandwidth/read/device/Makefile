SRCS = tb_read.cl

XML = read.xml
OBJ = read.aoco
LIB = read.aoclib

compile:
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -report -c -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS)

gen:clean
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -no-interleaving DDR -report -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_read.aocx

a10pl4:clean
	srun -p syn2 -w ppxsyn02 aoc -c $(XML) -o $(OBJ)
	srun -p syn2 -w ppxsyn02 aocl library create -o $(LIB) $(OBJ)
	srun -p syn2 -w ppxsyn02 aoc -board=a10pl4_dd4gb_gx115_m512 -no-interleaving DDR -report -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_read.aocx

emu:
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -march=emulator -report -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_read.aocx

clean:
	rm -rf $(OBJ) $(LIB) ./read ./tb_read tb_read.aoco tb_read.aocx ./.emu_models __all_sources.cl Makefile.efisim efi_testbench.sv
