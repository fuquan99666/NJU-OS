NAME := kernel
SRCS := kernel.c
export MODULE  := L0
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
# export ARCH := x86_64-qemu
# export ARCH := native
export ARCH := x86-qemu
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk

export CROSS_COMPILE := 
## image: git
