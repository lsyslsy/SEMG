# Comment/uncomment the following line to disable/enable debugging
DEBUG = n


# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

obj-m := rs485.o 
CROSS_COMPILE=arm-linux-
KERNELDIR ?= /home/yao/share/linux-2.6.36
PWD := $(shell pwd)
EXTRA_CFLAGS += $(DEBFLAGS)
all:modules test
modules:
	$(MAKE) -Wall -C  $(KERNELDIR) M=$(PWD) modules
test:rs485_test.c rs485_test.h
	arm-linux-gcc -Wall rs485_test.c -o rs485_test 
install:
	cp rs485.ko /forlinx/share
	cp rs485_test /forlinx/share
clean:
	rm -rf  *.o .*.cmd *.ko *.mod.c .tmp_versions rs485_test *.symvers *.order
.PHONY:modules clean install all
