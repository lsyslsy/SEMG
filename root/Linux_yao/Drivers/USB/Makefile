# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y
ARM=yy

# Add your debugging flag (or not) to CFLAGS
# ifeq ($(DEBUG),y)
#   DEBFLAGS = -O -g 
# else
#   DEBFLAGS = -O2
# endif

obj-m := usb_semg.o 

ifeq ($(ARM), y)
	CROSS_COMPILE=arm-linux-
	KERNELDIR ?= /home/yao/share/linux-2.6.36
	CCC=arm-linux-gcc
	#EXTRA_CFLAGS += -DSEMG_ARM
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	CCC=gcc
endif

PWD := $(shell pwd)
# EXTRA_CFLAGS += $(DEBFLAGS)
all: modules test
modules: usb_semg.c usb_semg.h
	$(MAKE) -Wall  -C $(KERNELDIR) M=$(PWD) modules
test: usb_test.c led.c usb_semg.h led.h
	$(CCC) -Wall -lrt usb_test.c led.c -o usb_test
install:
	cp usb_semg.ko /forlinx/share
	cp usb_test /forlinx/share
clean:
	rm -rf  *.o .*.cmd *.ko *.mod.c .tmp_versions pwm_test *.symvers *.order
.PHONY:modules clean install all
