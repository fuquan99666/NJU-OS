NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
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
	@build/test