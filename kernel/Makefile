NAME           := kernel
# SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c" -not -name "pmm_1.c")
INC_PATH       := include/ framework/

export AM_HOME := $(PWD)/../abstract-machine

export ARCH := 

ifeq ($(ARCH),)
export ARCH    := x86_64-qemu
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk

CROSS_COMPILE := 
# image: git

.PHONY : test


test:
	@gcc src/pmm.c test/test.c \
		-Iframework -Itest -DTEST -lpthread \
		-o build/test

test_basic: test
		@build/test 

testall: test
		@build/test 0	
		@build/test 1	
		@build/test 2	
		@build/test 3	
		@build/test 4