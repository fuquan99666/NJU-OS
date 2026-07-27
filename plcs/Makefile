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

test: plcs_0 plcs_3
	@echo "Testing sequential implementation"
	@python help.py | ./plcs_0
	@echo " "
	@echo "Testing parallel implementation"
	@python help.py | ./plcs_3 8

test_full: plcs_0 plcs_3
	@python3 test_plcs.py

a.out : plcs.c
	gcc plcs.c 

plcs_0 : plcs_0.c
	gcc -mcmodel=large plcs_0.c -o plcs_0

plcs_2: plcs_2.c
	gcc -mcmodel=large plcs_2.c -o plcs_2 

plcs_3: plcs_3.c
	gcc -mcmodel=large plcs_3.c -o plcs_3