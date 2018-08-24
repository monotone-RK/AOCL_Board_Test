NAME = hello_world
SRCS = $(NAME).cl

compile:
	aoc -report -c -save-temps -dot -Werror -g -v $(SRCS)

gen:clean
	aoc -report -save-temps -dot -Werror -g -v $(SRCS) -o ../bin/$(NAME).aocx

a10pl4:clean
	srun -p syn2 -w ppxsyn02 aoc -board=a10pl4_dd4gb_gx115_m512 -report -save-temps -dot -Werror -g -v $(SRCS) -o ../bin/$(NAME).aocx

emu:
	aoc -march=emulator -report -save-temps -dot -Werror -g -v $(SRCS) -o ../bin/$(NAME).aocx

clean:
	rm -rf ./$(NAME) $(NAME).aoco $(NAME).aocx ./.emu_models __all_sources.cl
