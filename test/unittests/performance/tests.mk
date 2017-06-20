#
# this makefile can be use to build and run the entire suite 
#
#     make -f tests.mk SOLC=solc ETHVM=ethvm EVM=evm all
#
# or build and run only a single test
#
#     make -f tests.mk SOLC=solc ETHVM=ethvm EVM=evm pop.bin
#
# or build but not run the entire suite 
#
#     make -f tests.mk SOLC=solc all
#
# or many other such possibilities
#

# overide these if desired
ifndef STATS
	STATS = time --format "stats: %U user %S system %E elapsed %P CPU %Mk res mem\n"
endif
ifndef CCC
	CCC = gcc -O0 -o
endif

# define path to these programs to pick one or more of them to run
# the default is to do nothing
ifndef SOLC
	SOLC=:
endif
ifndef ETHVM
	TEST_ETHVM=:
else
	TEST_ETHVM = $(STATS) $(ETHVM) test
endif
ifndef EVM
	TEST_EVM=:
else
	TEST_EVM = $(STATS) $(EVM) --codefile 
endif

%.bin : %.sol
	$(SOLC) -o . --overwrite --asm --bin $^ 
	$(TEST_ETHVM) $@
	$(TEST_EVM) $@ run

mul64c: mul64c.c
	$(CCC) mul64c mul64c.c
	$(STATS) ./mul64c

ops : \
	pop.bin \
	add64.bin \
	add128.bin \
	add256.bin \
	sub64.bin \
	sub128.bin \
	sub256.bin \
	mul64.bin \
	mul128.bin \
	mul256.bin \
	div64.bin \
	div128.bin \
	div256.bin

programs : \
	loop.bin \
	rng.bin \
	fun.bin \
	rc5.bin \
	mix.bin
	
all: ops mul64c programs


.PHONY : clean
clean :
	rm *.bin *.evm mul64c
