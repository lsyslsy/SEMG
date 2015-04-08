#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "led.h"

int Led_on(int Led_num)
{
    int fd = -1;
    if (Led_num != LED_1 && Led_num != LED_2 && Led_num != LED_3 && Led_num != LED_4)
     	return -1;
    fd = open("/dev/leds", O_RDWR);  // 打开设备
    if (fd < 0) {
        printf("Can't open /dev/leds\n");
        return -1;
    }
    ioctl(fd, LED_ON, Led_num);    // 点亮它
    close(fd);
    return 1;
}

int Led_off(int Led_num)
{
    int fd = -1;
    if (Led_num != LED_1 && Led_num != LED_2 && Led_num != LED_3 && Led_num != LED_4)
     	return -1;
    fd = open("/dev/leds", O_RDWR);  // 打开设备
    if (fd < 0) {
        printf("Can't open /dev/leds\n");
        return -1;
    }
    ioctl(fd, LED_OFF, Led_num);   // 熄灭它 
    close(fd);
    return 1;
}


