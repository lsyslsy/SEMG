/*
 * pwm sync module
 * set the pwm pin as pwm tout mode 
 * 1. Write the initial value into TCNTBn and TCMPBn.
 * 2. Set the manual update bit of the corresponding timer.
 *    (Recommended setting the inverter on/off bit (whether using inverter or not))
 * 3. Set the start bit of the corresponding timer to start the timer and clear only manual update bit.
 *
 * Frequency = PCLK/({prescaler value + 1})/{divider value}
 * {prescaler value} = 1~255
 * {divider value} =1,2,4,8,16,TCLK
 *
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
#include <plat/regs-timer.h>
#include <mach/gpio-bank-f.h>
//#include <mach/gpio-bank-l.h>
#include <mach/regs-gpio.h>
#include <mach/irqs.h>
#include "pwm_sync.h"
#include <mach/map.h>
#include <asm/hardware/vic.h>
 
int pwm_major = 200;
int pwm_minor = 0;
int pwm_dev_nr = 1;	/*device count*/


MODULE_AUTHOR("Yao");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:0.1");
MODULE_DESCRIPTION("Used for producing sync signal");

atomic_t pwm_sync_use_cnt = ATOMIC_INIT(1);//独占设备，只能打开一次
int pwm_sync_irq = IRQ_TIMER1;//_VIC;	/* irq number required */
struct fasync_struct *async_queue;		/* 异步通知队列 */

void pwm_sync_do_tasklet(unsigned long);
DECLARE_TASKLET(pwm_sync_tasklet, pwm_sync_do_tasklet, 0);

/*
 * 设置定时器的频率，为启动定时器做准备
 * PCLK 应该是66.5MHz 即66500000Hz
 * PWM clock = PCLK/({prescaler value + 1})/{divider value} 
 * pwm_clock = 66.5MHz/(4+1)/2=6.65MHz
 * ;
 */
int pwm_sync_set_freq(unsigned long freq)
{
	struct clk *clk_p;
	unsigned long pclk, temp;
	//unsigned int prescaler, divider;
	//setting clock
	clk_p = clk_get(NULL, "pclk");
	if (IS_ERR(clk_p)) {
		printk(KERN_ERR "get pclk failed\n");
		return -1;
	}
	pclk = clk_get_rate(clk_p);
	/* set prescaler=32 */
	temp = __raw_readl(S3C_TCFG0);
	temp &= ~S3C_TCFG_PRESCALER0_MASK;
	temp |= 4 << S3C_TCFG_PRESCALER0_SHIFT;
	__raw_writel(temp, S3C_TCFG0);
	/* set divider=2 */
	temp = __raw_readl(S3C_TCFG1);
	temp &= ~S3C_TCFG1_MUX1_MASK;
	temp |= S3C_TCFG1_MUX1_DIV2;
	__raw_writel(temp, S3C_TCFG1);

	/* set pwm sync frequency */
	//663200
	temp = 663200;//pclk/10 / freq;		/*  */
	__raw_writel(temp, S3C_TCNTB(1));
	__raw_writel(temp/2, S3C_TCMPB(1));

	wmb();

	temp = __raw_readl(S3C_TCON);
	/* invert-off设置电平相位默认为高定平，关闭autoreload, disable deadzone */
	temp = temp & ~(S3C_TCON_T1RELOAD | S3C_TCON_T1INVERT);		
	temp = temp | S3C_TCON_T1MANUALUPD;			/* set manual update to update new value from count buffer register */
	//mb();		/* seems no effect*/
	__raw_writel(temp, S3C_TCON);
	wmb();		/* 写入屏障，不知道对6410是否有必要，为了科学性加上 */
	temp &= ~S3C_TCON_T1MANUALUPD; /* clear mannual update bit */
	__raw_writel(temp, S3C_TCON);
	return 0;
}

void pwm_sync_start(void)
{
	unsigned long temp;

	/* 设置GPF15 为PWM输出，也可以用__raw_writel，但该函数实现了对gpio的锁定，各方面更佳，除了效率低 */
	s3c_gpio_cfgpin(S3C64XX_GPF(15), S3C_GPIO_SFN(2));		
	
	/*开启定时器*/
	temp = __raw_readl(S3C_TCON);
	temp = temp | S3C_TCON_T1RELOAD | S3C_TCON_T1START;			/* set autoreload mode and start */
	//mb();		/* seems no effect */
	__raw_writel(temp, S3C_TCON);

}

void pwm_sync_stop(void)
{
	unsigned long temp;
	temp = __raw_readl(S3C_TCON);
	/* 关闭autoreload, stop timer */
	temp &= ~(S3C_TCON_T1RELOAD | S3C_TCON_T1START);	
	__raw_writel(temp, S3C_TCON);
	s3c_gpio_cfgpin(S3C64XX_GPF(15), S3C_GPIO_INPUT);	/* stop pwm output */	
}

int pwm_sync_open (struct inode *inode, struct file *filp)
{
	
	if (!atomic_dec_and_test(&pwm_sync_use_cnt)) {
		atomic_inc(&pwm_sync_use_cnt);
		return -EBUSY;		/* 已经被打开过了 */
	}
	return 0;
}

long pwm_sync_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
	//int err = 0, tmp;
	int retval = 0;
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != PWM_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > PWM_IOC_NR) return -ENOTTY;


	switch(cmd) {
		case PWM_IOC_START:
			pwm_sync_start();
			break;
		case PWM_IOC_STOP:
			pwm_sync_stop();
			break;
		case PWM_IOC_SET_FREQ:
			retval = pwm_sync_set_freq(arg);
			break;
		default:
			return -ENOTTY;
	}

	return retval;

}

static int pwm_sync_fasync(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &async_queue);
}

static int pwm_sync_release(struct inode *inode, struct file *filp)
{
	unsigned long temp;
	pwm_sync_stop();
	atomic_inc(&pwm_sync_use_cnt);
	/* remove this filp from the asynchronously notified filp's */
	pwm_sync_fasync(-1, filp, 0);
	return 0;
}

static struct file_operations pwm_sync_ops ={
	.owner = THIS_MODULE,
	.open = pwm_sync_open,
	.release = pwm_sync_release,
	.unlocked_ioctl = pwm_sync_ioctl,
	.fasync = pwm_sync_fasync
};
// void producing
// {
// 	print tcntb0 tcnpb0 value
// print pclk tcfg0  tcfg1 hz
// }

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "pwm_sync",
	.fops = &pwm_sync_ops
};

void pwm_sync_do_tasklet(unsigned long unused)
{
	if(async_queue)
		kill_fasync(&async_queue, SIGIO, POLL_IN);	/* 发送异步通知 */
		
}
/*
 * 很奇怪的是CSTAT不用清除状态，而且读出来为10
 */
static irqreturn_t pwm_sync_interrupt(int irq, void *dev_id)
{
	/* used in shared irq, but not used here*/
	unsigned long temp;
	// printk(KERN_INFO "TCON:%lx\n", 11);
	//temp = readl(S3C_TINT_CSTAT);
	//print_regs();
	// if(!(temp & S3C_TINT_CSTAT_T1INT))	/* no interrupt */
	// 	return IRQ_NONE;
	// temp |= S3C_TINT_CSTAT_T1INT;		/* clear T1 status */
	// __raw_writel(temp, S3C_TINT_CSTAT);		
	// printk(KERN_INFO "TCON:%lx\n", 13);
	/* 翻转电平 */
	//unsigned long temp;
	// temp = readl(S3C64XX_GPLDAT);
	// temp ^= 0x0100;
	// writel(temp, S3C64XX_GPLDAT);

	tasklet_schedule(&pwm_sync_tasklet);

	return IRQ_HANDLED;
}


void print_regs(void)
{
	unsigned long temp;
	temp = __raw_readl(S3C_TCON);
	printk(KERN_INFO "TCON:%lx\n", temp);
	temp = __raw_readl(S3C_TCFG0);
	printk(KERN_INFO "TCFG0:%lx\n", temp);
	temp = __raw_readl(S3C_TCFG1);
	printk(KERN_INFO "TCFG1:%lx\n", temp);
	temp = __raw_readl(S3C_TINT_CSTAT);
	printk(KERN_INFO "TINT_CSTAT:%lx\n", temp);
	temp = __raw_readl(S3C_TCNTB(4));
	printk(KERN_INFO "TCNT:%lx\n", temp);
	
}

static int __init pwm_sync_init_module(void)
{
	unsigned long temp;
	//dev_t devno;
	unsigned long base = 0x7F006000;
	int err;
	if (!request_mem_region(base, 4, "pwm_syn")) {
		printk(KERN_ERR "request I/O memory for pwm_sync failed\n");
		return -ENODEV;
	}
	base = (unsigned long) ioremap(base, 1);
	printk(KERN_EMERG "phy :0x7F006000  to  %lx\n ",base);
	print_regs();
	err = misc_register(&misc);
	if(err)	{
		iounmap((void __iomem *)base);
		release_mem_region(base, 4);
		return err;
	}

	err= request_irq(pwm_sync_irq, pwm_sync_interrupt, IRQF_DISABLED, "pwm_sync", NULL);
	if(err) {
		printk(KERN_INFO "pwm_sync: can't get assigned irq %d\n",
				pwm_sync_irq);
		pwm_sync_irq = -1;
	}
	else { /* actually enable it  */
		temp = __raw_readl(S3C_TINT_CSTAT);
		temp |= S3C_TINT_CSTAT_T1INTEN;		/* enable T1 timer interrupt */
		__raw_writel(temp, S3C_TINT_CSTAT);	

		temp = __raw_readl(VA_VIC0 + VIC_INT_ENABLE);
		temp |= 1 << 24;		/* Open the VIC T1 timer interrupt */
		__raw_writel(temp, VA_VIC0 + VIC_INT_ENABLE);	
	}
	print_regs();
	return 0;
}

static void __exit pwm_sync_cleanup_module(void)
{
	unsigned long temp;
	unsigned long base = 0x7F006000;
	if(pwm_sync_irq > 0) {
		temp = __raw_readl(S3C_TINT_CSTAT);
		temp |= S3C_TINT_CSTAT_T1INT;		/* clear T1 status */
		temp &= ~S3C_TINT_CSTAT_T1INTEN;		/* disable T1 timer interrupt */
		__raw_writel(temp, S3C_TINT_CSTAT);	

		temp = __raw_readl(VA_VIC0 + VIC_INT_ENABLE_CLEAR);
		temp |= 1 << 24;		/* enable T1 timer interrupt */
		__raw_writel(temp, VA_VIC0 + VIC_INT_ENABLE_CLEAR);	

		free_irq(pwm_sync_irq, NULL);
	}
	tasklet_disable(&pwm_sync_tasklet);/*会等到tasklet执行完*/
	iounmap((void __iomem *)base);
	release_mem_region(base, 4);
	misc_deregister(&misc);
}

module_init(pwm_sync_init_module);
module_exit(pwm_sync_cleanup_module);

