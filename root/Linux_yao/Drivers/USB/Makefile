# Comment/uncomment the following line to disable/enable debugging
DEBUG = y


# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g  
else
  DEBFLAGS = -O2
endif

obj-m := usb_semg.o 
#CROSS_COMPILE=arm-linux-
#KERNELDIR ?= /home/yao/share/linux-2.6.36
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
EXTRA_CFLAGS += $(DEBFLAGS)
all:modules
modules:
	$(MAKE) -Wall  -C $(KERNELDIR) M=$(PWD) modules
#test:usb_seng.c
#	$(CC) -Wall pwm_test.c -o pwm_test 
install:
#	cp pwm_sync.ko /forlinx/share
#	cp pwm_test /forlinx/share
clean:
	rm -rf  *.o .*.cmd *.ko *.mod.c .tmp_versions pwm_test *.symvers *.order
.PHONY:modules clean install all
