# Comment/uncomment the following line to disable/enable debugging
DEBUG = n


# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

obj-m := pwm_sync.o 
CROSS_COMPILE=arm-linux-
KERNELDIR ?= /home/yao/share/linux-2.6.36
PWD := $(shell pwd)
EXTRA_CFLAGS += $(DEBFLAGS)
all:modules test
modules:
	$(MAKE) -Wall -C  $(KERNELDIR) M=$(PWD) modules
test:pwm_test.c pwm_sync.h
	arm-linux-gcc -Wall pwm_test.c -o pwm_test 
install:
	cp pwm_sync.ko /forlinx/share
	cp pwm_test /forlinx/share
clean:
	rm -rf  *.o .*.cmd *.ko *.mod.c .tmp_versions pwm_test *.symvers *.order
.PHONY:modules clean install all
