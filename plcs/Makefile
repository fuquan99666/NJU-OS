NAME := $(shell basename $(PWD))
export MODULE := M2
all: $(NAME)-64 $(NAME)-32
LDFLAGS += -lpthread

include ../Makefile


test_0: a.out plcs_0
	@echo "Testing parallel implementation"
	@echo "QWERTYUIOPASDFGHJKLZXCVBNMQWERTYUIOPASDFGHJKLZXCVBNMQWERTYUIOPASDFGHJKLZXCVBNM ASDFGHJKLPOIUYTREWQMNBVCXZASDFGHJKLPOIUYTREWQMNBVCXZASDFGHJKLPOIUYTREWQMNBVCXZ" | ./a.out 4
	@echo " "
	@echo "Testing sequential implementation"
	@echo "QWERTYUIOPASDFGHJKLZXCVBNMQWERTYUIOPASDFGHJKLZXCVBNMQWERTYUIOPASDFGHJKLZXCVBNM ASDFGHJKLPOIUYTREWQMNBVCXZASDFGHJKLPOIUYTREWQMNBVCXZASDFGHJKLPOIUYTREWQMNBVCXZ" | ./plcs_0

test: a.out plcs_0 
	@echo "Testing parallel implementation"
	@python help.py | ./a.out 8
	@echo " "
	@echo "Testing sequential implementation"
	@python help.py | ./plcs_0

a.out : plcs.c
	gcc plcs.c 

plcs_0 : plcs_0.c
	gcc plcs_0.c -o plcs_0