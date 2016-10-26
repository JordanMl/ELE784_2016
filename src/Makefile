# Variables pointing to different paths
#KERNEL_DIR    	:= /lib/modules/$(shell uname -r)/build
KERNEL_DIR    	:= /usr/src/linux-headers-3.16.0

PWD        	:= $(shell pwd)

obj-m        := charDriver.o

all: ele784_driver

# We build our module in this section
ele784_driver:
	-cd ..
	@mkdir bin
	-cd ./bin
	@echo "Building the ELE784 Lab1: Ring buffer driver"
	@make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

# It's always a good thing to offer a way to cleanup our development directory
clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c Module.symvers modules.order
	-rm -rf .tmp_versions
