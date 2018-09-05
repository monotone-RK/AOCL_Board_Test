SRCS = tb_wait_func.cl

XML = wait_func.xml
OBJ = wait_func.aoco
LIB = wait_func.aoclib

compile:
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -report -c -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS)

flat:clean
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -bsp-flow=flat -report -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_wait_func.aocx

gen:clean
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -report -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_wait_func.aocx

a10pl4:clean
	srun -p syn2 -w ppxsyn02 aoc -c $(XML) -o $(OBJ)
	srun -p syn2 -w ppxsyn02 aocl library create -o $(LIB) $(OBJ)
	srun -p syn2 -w ppxsyn02 aoc -board=a10pl4_dd4gb_gx115_m512 -report -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_wait_func.aocx

emu:
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -march=emulator -report -save-temps -dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_wait_func.aocx

clean:
	rm -rf $(OBJ) $(LIB) ./wait_func ./tb_wait_func tb_wait_func.aoco tb_wait_func.aocx ./.emu_models __all_sources.cl Makefile.efisim efi_testbench.sv
