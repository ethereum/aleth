#
# this makefile can be use to build and run the entire suite 
#
#     make -f tests.mk SOLC=solc ETHVM=ethvm EVM=evm PARITY=parity-evm all
#
# or build and run only a single test on a single VM
#
#     make -f tests.mk SOLC=solc ETHVM=ethvm pop.ran
#
# or build but not run the entire suite 
#
#     make -f tests.mk SOLC=solc all
#
# or many other such possibilities
#
# define path to these programs to pick one or more of them to run
# the default is to do nothing but print the command line
#
ifndef SOLC
	SOLC_=:
else
	SOLC_=$(SOLC)
endif
ifndef ETHVM
	ETHVM_=:
else
	ETHVM_ = $(STATS) $(ETHVM) --network Homestead test
endif
ifndef EVM
	EVM_=:
else
	EVM_ = $(STATS) $(EVM) --codefile 
endif
ifndef PARITY
	PARITY_=:
else
	PARITY_ = $(STATS) $(PARITY) stats --gas 10000000000 --code 
endif

# overide these if desired
ifndef STATS
	STATS = time --format "stats: %U user %S system %E elapsed %P CPU %Mk res mem\n"
endif
ifndef CCC
	CCC = gcc -O0 -o
endif

%.ran : %.bin
	$(ETHVM_) $*.bin; touch $*.ran
	$(EVM_) $*.bin run; touch $*.ran
	$(PARITY_) `cat $*.bin`; touch $*.ran

%.bin : %.asm
	$(SOLC_) --assemble $*.asm | grep '^[0-9a-f]\+$\' > $*.bin

%.bin : %.sol
	$(SOLC_) -o . --overwrite --asm --bin $*.sol 
	
mul64c: mul64c.c
	$(CCC) mul64c mul64c.c
	$(STATS) ./mul64c

ops : \
	nop.ran \
	pop.ran \
	popa.ran \
	add64.ran \
	add128.ran \
	add256.ran \
	sub64.ran \
	sub128.ran \
	sub256.ran \
	mul64.ran \
	mul128.ran \
	mul256.ran \
	div64.ran \
	div128.ran \
	div256.ran

programs : \
	loop.ran \
	rng.ran \
	fun.ran \
	rc5.ran \
	mix.ran
	
all: ops mul64c programs


.PHONY : clean
clean :
	rm *.ran *.bin *.evm mul64c
