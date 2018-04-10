NAME = nop
SRCS = $(NAME).cl

compile:
	aoc --report -c --save-temps --dot -Werror -g -v $(SRCS)

gen:clean
	aoc --report --save-temps --dot -Werror -g -v $(SRCS) -o ../bin/$(NAME).aocx

emu:
	aoc -march=emulator --report --save-temps --dot -Werror -g -v $(SRCS) -o ../bin/$(NAME).aocx
	# CL_CONTEXT_EMULATOR_DEVICE_ALTERA=1 bin/host $(NAME) 1024

clean:
	rm -rf ./$(NAME) $(NAME).aoco $(NAME).aocx ./.emu_models __all_sources.cl
