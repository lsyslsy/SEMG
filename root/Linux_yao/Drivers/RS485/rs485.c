/*
 * rs485 module
 *
 * GPK10 is connected KP_ROW2 and \R\E Receiver Output Enable when RE=0
 * GPK11 is connected KP_ROW3 and DE   Driver Output Enable when DE=1
 * 本来用ioreadl/iowritel 比readl/write还是用好，但不好读头文件定义的绝对地址，太麻烦了，
 * 最后还是用__raw_readl __raw_writel好，内核三星驱动都是用这个
 * 但需要注意的是，这些函数使用的都是虚拟地址，貌似头文件都帮我们弄好了，也可以用ioremap进行映射
 * 用timer1 作为sync 信号
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/signal.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio-bank-f.h>
//#include <mach/gpio-bank-l.h>
#include <mach/regs-gpio.h>
#include <mach/irqs.h>
#include "rs485.h"
#include <mach/map.h>
#include <asm/hardware/vic.h>
 
int pwm_major = 200;
int pwm_minor = 0;
int pwm_dev_nr = 1;	/*device count*/


MODULE_AUTHOR("Yao C&S ZJU");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:0.1");
MODULE_DESCRIPTION("Used for rs485 communicate");

atomic_t rs485_use_cnt = ATOMIC_INIT(1);//独占设备，只能打开一次

int rs485_open (struct inode *inode, struct file *filp)
{
	unsigned long temp;
	if (!atomic_dec_and_test(&rs485_use_cnt)) {
		atomic_inc(&rs485_use_cnt);
		return -EBUSY;		/* 已经被打开过了 */
	}
	return 0;
}

long rs485_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
	unsigned long temp;
	int retval = 0;
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != RS485_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > RS485_IOC_NR) return -ENOTTY;


	switch(cmd) {
		case RS485_SEND_MODE:
			temp = __raw_readl(S3C_64XX_GPKDAT);
			temp &= ~0x0C00			/* set default recv mode */
			__raw_writel(temp, S3C_64XX_GPKDAT);
			break;
		case RS485_RECV_MODE:
			temp = __raw_readl(S3C_64XX_GPKDAT);
			temp |= 0x0C00			/* set default recv mode */
			__raw_writel(temp, S3C_64XX_GPKDAT);
			break;
		default:
			return -ENOTTY;
	}

	return retval;

}

static int rs485_release(struct inode *inode, struct file *filp)
{
	atomic_inc(&rs485_use_cnt);
	return 0;
}

static struct file_operations rs485_ops ={
	.owner = THIS_MODULE,
	.open = rs485_open,
	.release = rs485_release,
	.unlocked_ioctl = rs485_ioctl,

};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rs485",
	.fops = &rs485_ops
};



static int __init rs485_init_module(void)
{
	unsigned long temp;
	//dev_t devno;

	int err;

	s3c_gpio_cfgpin(S3C64XX_GPK(10), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S3C64XX_GPK(10), S3C_GPIO_PULL_UP);

	s3c_gpio_cfgpin(S3C64XX_GPK(11), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S3C64XX_GPK(11), S3C_GPIO_PULL_UP);

	temp = __raw_readl(S3C_64XX_GPKDAT);
	temp &= ~0x0C00			/* set default recv mode */
	__raw_writel(temp, S3C_64XX_GPKDAT);

	err = misc_register(&misc);
	if(err)	{
		return err;
	}

	return 0;
}

static void __exit rs485_cleanup_module(void)
{

	misc_deregister(&misc);
}

module_init(rs485_init_module);
module_exit(rs485_cleanup_module);

