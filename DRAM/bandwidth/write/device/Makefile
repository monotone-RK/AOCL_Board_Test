SRCS = tb_write.cl

XML = write.xml
OBJ = write.aoco
LIB = write.aoclib

compile:
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc --report -c --save-temps --dot -Werror -g -v -l $(LIB) $(SRCS)

gen:clean
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -no-interleaving=DDR --report --save-temps --dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_write.aocx

emu:
	aoc -c $(XML) -o $(OBJ)
	aocl library create -o $(LIB) $(OBJ)
	aoc -march=emulator --report --save-temps --dot -Werror -g -v -l $(LIB) $(SRCS) -o ../bin/tb_write.aocx
	# CL_CONTEXT_EMULATOR_DEVICE_ALTERA=1 bin/host tb_write 1024

clean:
	rm -rf $(OBJ) $(LIB) ./write ./tb_write tb_write.aoco tb_write.aocx ./.emu_models __all_sources.cl
