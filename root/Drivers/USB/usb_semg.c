/*
 * USB SEMG driver
 * Copyright (C) 2014 Shangyao Lin (1210tom@163.com)
 * refer to: Konicawc.c, usb-skeleton.c, Uvc_video.c
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <asm/atomic.h>

#include "usb_semg.h"

#ifdef SEMG_ARM
#include <asm/io.h>
#include <mach/map.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-bank-m.h>
#endif
//#define ASYNC_NOTIFY_SUPPORT
//#define POLL_SUPPORT

//TODO
#define USB_SEMG_VENDOR_ID 		0x05AC
#define USB_SEMG_PRODUCT_ID 	0x2222

#define SEMG_FRAME_SIZE 		3258
#define ISOC_IN_BUFFER_SIZE 	4096		/* 页大小为4096，验证过 */

/* usb设备的次设备号起始 */
#define USB_SEMG_MINOR_BASE 	199

/* supported usb device table */
static struct usb_device_id semg_supported_table [] = {
	{ USB_DEVICE(USB_SEMG_VENDOR_ID, USB_SEMG_PRODUCT_ID) },
	{}
};
MODULE_DEVICE_TABLE(usb, semg_supported_table);

struct usb_semg {
	atomic_t open_cnt; 					/* 剩余可打开次数，仅可打开一次，每打开一次减1 */
	struct usb_device 	*udev;			/* usb device */
	struct usb_interface 	*interface;  /* usb interface 的引用*/

	__u8            isoc_in_endpointAddr;
	size_t 			isoc_in_size;		/* the size of the receive buffer */
	size_t			isoc_in_filled;		/* number of bytes in the buffer */
	size_t			isoc_in_copied;		/* already copied to user space */
	size_t 			isoc_in_packetsize;		/* Max Packet size of isoc endpoint */
	size_t 			isoc_in_npackets;	/* number of packets: must be 4 */
	void 	*isoc_in_buffer;
	dma_addr_t	isoc_in_buffer_dma;
	struct urb 		*isoc_in_urb;		/* 用来读取数据的urb*/
	struct completion 	isoc_in_completion; 	/* completion for read */

	__u8            bulk_in_endpointAddr;
	size_t 			bulk_in_size;		/* the size of the receive buffer */
	size_t			bulk_in_filled;		/* number of bytes in the buffer */
	size_t			bulk_in_copied;		/* already copied to user space */
	size_t 			bulk_in_packetsize;		/* Max Packet size of isoc endpoint */
	size_t 			bulk_in_npackets;	/* number of packets: must be 4 */
	void 	*bulk_in_buffer;
	dma_addr_t	bulk_in_buffer_dma;
	struct urb 		*bulk_in_urb;		/* 用来读取数据的urb*/
	struct completion 	bulk_in_completion; 	/* completion for read */


	spinlock_t 	err_lock; 			//lock for errors
	int 			errors; 			//上次传输的错误结果
	struct kref 	kref;				/* 设备引用计数 */
#ifdef ASYNC_NOTIFY_SUPPORT
	struct fasync_struct 	*async_queue; /* 异步通知队列 */
#endif
	//atomic_t /* 跟踪打开计数 */
	struct mutex 	io_mutex; 			/* 用于读写，断开之间的同步 */
};
#define to_semg_dev(p) container_of(p, struct usb_semg, kref)

static struct usb_driver semg_driver;//static类型的前向声明，长见识了吧。因为后面也有定义并初始化了

static void semg_delete(struct kref *kref) {
	struct usb_semg *dev = to_semg_dev(kref);

	usb_free_urb(dev->isoc_in_urb);
	usb_free_urb(dev->bulk_in_urb);
	usb_put_dev(dev->udev);
	if (dev->isoc_in_buffer != NULL)
		usb_free_coherent(dev->udev, dev->isoc_in_size, dev->isoc_in_buffer, //函数会处理isoc_in_buffer==NULl 的情况
					dev->isoc_in_buffer_dma);
	if (dev->bulk_in_buffer != NULL)
		usb_free_coherent(dev->udev, dev->bulk_in_size, dev->bulk_in_buffer, //函数会处理isoc_in_buffer==NULl 的情况
					dev->bulk_in_buffer_dma);
}


static void semg_read_bulk_callback(struct urb *urb)
{
	struct usb_semg *dev = urb->context;
	int i;
	unsigned int tmp;
#ifdef SEMG_ARM
	tmp = readl(S3C64XX_GPMDAT);
	tmp |= 1;
	writel(tmp, S3C64XX_GPMDAT);
#endif
	spin_lock(&dev->err_lock);

/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			dev_err(&dev->interface->dev, "%s - nonzero read bulk status received: %d",
			    __func__, urb->status);

		dev->errors = urb->status;
	} else {
		//dev->isoc_in_filled = 0;
		dev->bulk_in_filled = urb->actual_length;//TODO = or +=
	}

	spin_unlock(&dev->err_lock);
#ifdef ASYNC_NOTIFY_SUPPORT
	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
#endif

	complete(&dev->bulk_in_completion);
}

static void semg_read_isoc_callback(struct urb *urb)
{
	struct usb_semg *dev = urb->context;
	int i;
	unsigned int tmp;
#ifdef SEMG_ARM
	tmp = readl(S3C64XX_GPMDAT);
	tmp |= 1;
	writel(tmp, S3C64XX_GPMDAT);
#endif
	spin_lock(&dev->err_lock);
	/* sync/async unlink faults aren't errors */
	//TODO
	if (urb->error_count > 0) {
		dev->errors = -1;
		 dev_err(&dev->interface->dev,
		 			"%s - failed transfer count %d\n",
                    __func__, urb->error_count);
	} else {
		dev->isoc_in_filled = 0;
		for (i = 0; i < urb->number_of_packets; i++) {
			// dev_info(&dev->interface->dev, " frame %i received size :%zd;status: %d\n",
			// 	i, urb->iso_frame_desc[i].actual_length, urb->iso_frame_desc[i].status);
			dev->isoc_in_filled += urb->iso_frame_desc[i].actual_length;
		}

	}

	/*if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			err("%s - nonzero write bulk status received: %d",
			    __func__, urb->status);

		dev->errors = urb->status;
	} else {
		dev->iosc_in_filled = urb->actual_length;
	}*/

	spin_unlock(&dev->err_lock);
#ifdef ASYNC_NOTIFY_SUPPORT
	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
#endif

	complete(&dev->isoc_in_completion);
}
/*
 * Block Read
 */
static ssize_t semg_read(struct file *file, char __user *user_buf, size_t count, loff_t *pos) {

	struct usb_semg *dev = (struct usb_semg *)file->private_data;
	struct urb  *urb = dev->bulk_in_urb;
	unsigned int i;
	int retval = 0;
	unsigned int tmp;

	//dev->isoc_in_buffer = usb_alloc_coherent(dev->udev, )

	mutex_lock(&dev->io_mutex);

#ifdef SEMG_ARM
	tmp = readl(S3C64XX_GPMDAT);
	tmp &= ~1;
	writel(tmp, S3C64XX_GPMDAT);
#endif
	/* 初始化urb */
	// urb->dev = dev->udev;
	// urb->pipe = usb_rcvisocpipe(dev->udev, dev->isoc_in_endpointAddr);
	// urb->interval = 1;///TODO:not sure//yes sure
	// urb->context = dev;
	// urb->complete = semg_read_isoc_callback;
	// urb->transfer_buffer = dev->isoc_in_buffer;
	// urb->transfer_dma = dev->isoc_in_buffer_dma;
	// urb->number_of_packets = dev->isoc_in_npackets;
	// //urb->start_frame = usb_get_current_frame_number(urb->dev) - 10;
	// urb->transfer_buffer_length = dev->isoc_in_size;      //TODO   是不是该使用SEMG_FRAME_SIZE;
	// urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;//尽早开始同步传输，优选使用DMA传输
	// /* 设置isoc各帧的缓冲区位移，每帧预计长度 */
	// for (i = 0; i < urb->number_of_packets - 1; i++) {
	// 	urb->iso_frame_desc[i].offset = i * dev->isoc_in_packetsize;
	// 	//expected length, 实际读到的大小是由设备决定的，
	// 	//如果收到的数据长度>length，则会产生EOVERFLOW的错误;如果实际数据长度<length，则读到的acutal_length<length
	// 	urb->iso_frame_desc[i].length = dev->isoc_in_packetsize;
	// }
	// urb->iso_frame_desc[i].offset = i * dev->isoc_in_packetsize;
	// urb->iso_frame_desc[i].length = SEMG_FRAME_SIZE - i * dev->isoc_in_packetsize;

	// //dev_info(&dev->interface->dev, " %#x: packets of number:%zd, total size:%zd, max size:%zd", dev->isoc_in_endpointAddr,
	// //	dev->isoc_in_npackets , dev->isoc_in_size, dev->isoc_in_packetsize);
	usb_fill_bulk_urb(urb, dev->udev, usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr),
				dev->bulk_in_buffer, dev->bulk_in_size, semg_read_bulk_callback, dev);
	urb->transfer_dma = dev->bulk_in_buffer_dma;
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* submit urb */
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if (retval < 0) {
		 dev_err(&dev->interface->dev,
		 			"%s - failed submitting read urb, error %d\n",
                    __func__, retval);
		goto out;
	}
	/* submit urb */
	/*wait for read operation complete*/
	wait_for_completion(&dev->bulk_in_completion);
	//while (dev->isoc_in_filled != SEMG_FRAME_SIZE)
	//	msleep(1);
	//goto out;
	spin_lock(&dev->err_lock);
	/* errors must be reported */
	retval = dev->errors;
	if (retval < 0) {
		/* any error is reported once */
		dev->errors = 0;
		/* to preserve notifications about reset */
		retval = (retval == -EPIPE) ? retval : -EIO;
		/* no data to deliver */
		dev->bulk_in_filled = 0;
		spin_unlock(&dev->err_lock);
		/* report it */
		goto out;
	}
	spin_unlock(&dev->err_lock);

	//  只提供机制，不应该提供策略
	// if((dev->bulk_in_filled != SEMG_FRAME_SIZE) || (SEMG_FRAME_SIZE != count)) {
	// 	dev_err(&dev->interface->dev, "total received size :%zd\n", dev->bulk_in_filled);
	// 	retval = -EINVAL;
	// 	goto out;
	// }

	if (count > dev->bulk_in_filled)
		count = dev->bulk_in_filled;
	if(copy_to_user(user_buf, dev->bulk_in_buffer, count)) {
		retval = -EFAULT;
		goto out;
	}

	retval = count;

out:
	mutex_unlock(&dev->io_mutex);
	return retval;
}

static ssize_t semg_write(struct file *file, const char __user *uesr_buf, size_t count, loff_t *f_pos) {
	//读写之间要加锁
	struct usb_semg *dev = file->private_data;

	mutex_lock(&dev->io_mutex);

	mutex_unlock(&dev->io_mutex);
	return -1;
}

#ifdef POLL_SUPPORT
//TODO lack wait queue
static unsigned int semg_poll(struct file *filep, struct poll_table_struct *wait) {
	struct usb_semg *dev = filep->private_data;
	unsigned int mask = 0;

	mutex_lock(&dev->io_mutex);
	poll_wait(file, &, wait);

	if (dev->isoc_in_filled != 0)
		mask |= POLLIN | POLLRDNORM;
	mutex_unlock(&dev->io_mutex);


	return mask;
}
#endif

static int semg_open(struct inode *inode, struct file *file) {
	struct usb_interface *interface;
	struct usb_semg *dev;
	int retval = 0;
	int subminor;

	subminor = iminor(inode);//获取次设备号
	interface = usb_find_interface(&semg_driver, subminor);
	if (!interface) {				//可能usb设备刚被拔了
		pr_err("%s - error, can't find semg usb device for minor %d", __func__, subminor);
		retval = -ENODEV;
		goto error;
	}

	dev = usb_get_intfdata(interface); //可能usb设备刚被拔了
	if(!dev) {
		retval = -ENODEV;
		goto error;
	}

	if(!atomic_dec_and_test(&dev->open_cnt)) {
		atomic_inc(&dev->open_cnt);
		return -EBUSY;		/* 已经被打开过了 */
	}

	kref_get(&dev->kref);//引用加1

	mutex_lock(&dev->io_mutex);
//	iface->needs_remote_wakeup = 1;
	//从网上查到
	//Nowadays, the power manager for usb interfaces is transparent, so we shouldn't call
	//usb_autopm_get_interface to wakeup, neither usb_autopm_put_interface to suspend.
	//so usb_autopm_get_interface return EACCESS(13)
	// retval = usb_autopm_get_interface(interface);	/* 阻止usb设备autosuspended */
	// if (retval) {
	// 	atomic_inc(&dev->open_cnt);
	// 	mutex_unlock(&dev->io_mutex);
	// 	kref_put(&dev->kref, semg_delete);
	// 	goto error;
	// }

	file->private_data = dev;  //save dev for other functions use
	mutex_unlock(&dev->io_mutex);
	//if error
	//usb_autopm_put_interface

error:

	return retval;
}

#ifdef ASYNC_NOTIFY_SUPPORT
static int semg_fasync(int fd, struct file *filep, int mode) {
	struct usb_semg *dev = filep->private_data;
	return fasync_helper(fd, filep, mode, &dev->async_queue);

}
#endif

static int semg_release(struct inode *inode, struct file *file) {
	struct usb_semg *dev;

	dev = file->private_data;
	if (dev == NULL)	//怎么可能？
		return -ENODEV;

	atomic_inc(&dev->open_cnt);
	// mutex_lock(&dev->io_mutex);
	// if(dev->interface)
	// 	usb_autopm_put_interface(dev->interface);		/* 允许usb设备autosuspended */
	// mutex_unlock(&dev->io_mutex);

#ifdef ASYNC_NOTIFY_SUPPORT
	semg_fasync(-1, file, 0); 		//remove from asynchronous notification list
#endif

	/* decrement the count on our device */
	kref_put(&dev->kref, semg_delete);


	return 0;
}

static int semg_flush(struct file *file, fl_owner_t id) {
	return 0;
}

// TODO: jiffies 个数, 参见驱动书355上部,有bug,防止在调用时被断开
static int get_branch_num(struct file *filep) {
	int retval = 0;
	unsigned char n = 0;
	struct usb_semg *dev = filep->private_data;
	mutex_lock(&dev->io_mutex);
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), 0x62, 0xC0, 0, 0, &n, sizeof(n), 20);
	if (retval < 0)
		dev_err(&dev->interface->dev, "%s - getbranchnum failed: %d",
				    __func__, retval);
	// dev_info(&dev->interface->dev, "%s - received %d bytes, value:%d",
	// 			    __func__, retval, n);
	mutex_unlock(&dev->io_mutex);
	return retval < 0? retval: n;
}

// TODO: jiffies 个数, 参见驱动书355上部,有bug,防止在调用时被断开
static int set_delay(struct file *filep, __u16 ms) {
	int retval = 0;
	struct usb_semg *dev = filep->private_data;
	mutex_lock(&dev->io_mutex);
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), 0x63, 0x40, ms, 0, 0, 0, 20);
	if (retval < 0)
		dev_err(&dev->interface->dev, "%s - set delay failed: %d",
				    __func__, retval);
	// dev_info(&dev->interface->dev, "%s - send %d bytes, value:%d",
	// 			    __func__, retval, ms);
	mutex_unlock(&dev->io_mutex);
	return retval < 0 ? retval: 0;
}

// get expected frame number
static int get_expected_fn(struct file *filep) {
	int retval = 0;
	__u16 fn = 0; // frame number
	struct usb_semg *dev = filep->private_data;
	mutex_lock(&dev->io_mutex);
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), 0x65, 0xC0, 0, 0, &fn, sizeof(fn), 20);
	if (retval < 0)
		dev_err(&dev->interface->dev, "%s - get expected framenumber failed: %d",
				    __func__, retval);
	// else
	// 	dev_info(&dev->interface->dev, "%s - get expected framenumber, value:%d",
	// 			    __func__, fn);
	mutex_unlock(&dev->io_mutex);
	return retval < 0 ? retval: fn;
}

static int set_expected_fn(struct file *filep, __u16 framenumber) {
	int retval = 0;
	struct usb_semg *dev = filep->private_data;
	mutex_lock(&dev->io_mutex);
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), 0x64, 0x40, framenumber, 0, 0, 0, 20);
	if (retval < 0)
		dev_err(&dev->interface->dev, "%s - set expected framenumber failed: %d",
				    __func__, retval);
	// else
	// 	dev_info(&dev->interface->dev, "%s -set expected framenumber, value:%d",
	//			    __func__, framenumber);
	mutex_unlock(&dev->io_mutex);
	return retval < 0 ? retval: 0;
}

static int set_sample_period(struct file *filep, __u16 T) {
	int retval = 0;
	struct usb_semg *dev = filep->private_data;
	mutex_lock(&dev->io_mutex);
	retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), 0x61, 0x40, T, 0, 0, 0, 20);
	if (retval < 0)
		dev_err(&dev->interface->dev, "%s - set sample period failed: %d",
				    __func__, retval);
	// else
	// 	dev_info(&dev->interface->dev, "%s -set expected framenumber, value:%d",
	//			    __func__, framenumber);
	mutex_unlock(&dev->io_mutex);
	return retval < 0 ? retval: 0;
}

// get init state
// 100 200 300 400
static int get_init_state(struct file *filep) {
	int retval = 0;
	__u16 state = 0; // frame number
	struct usb_semg *dev = filep->private_data;
	mutex_lock(&dev->io_mutex);
	retval = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0), 0x66, 0xC0, 0, 0, &state, sizeof(state), 20);
	if (retval < 0)
		dev_err(&dev->interface->dev, "%s - get init state failed: %d",
				    __func__, retval);
	// dev_info(&dev->interface->dev, "%s - send %d bytes, value:%d",
	// 			    __func__, retval, ms);
	mutex_unlock(&dev->io_mutex);
	return retval < 0 ? retval: state;
}

// TODO: 确保在正确打开设备后才能ioctl
long semg_ioctl(struct file * filep, unsigned int cmd, unsigned long arg)
{
	//int err = 0, tmp;
	int retval = 0;
	struct usb_semg *dev = filep->private_data;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != USB_SEMG_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > USB_SEMG_IOC_NR) return -ENOTTY;


	switch(cmd) {
		case USB_SEMG_GET_BRANCH_NUM:
			return get_branch_num(filep);
		case USB_SEMG_SET_DELAY:
			if (arg <=0 || arg >= 1024)
				return -EINVAL;
			else
				return set_delay(filep, (__u16)arg);
		case USB_SEMG_SET_SAMPLEPERIOD:
			if (arg <= 10 || arg > 100)
				return -EINVAL;
			else
				return set_sample_period(filep, (__u16)arg);
		case USB_SEMG_GET_CURRENT_FRAME_NUMBER:
			return usb_get_current_frame_number(dev->udev) & 0x03ff;
		case USB_SEMG_GET_EXPECTED_FRAME_NUMBER:
			return get_expected_fn(filep);
		case USB_SEMG_SET_EXPECTED_FRAME_NUMBER:
			if (arg  >= 1024)
				return -EINVAL;
			else
				return set_expected_fn(filep, (__u16)arg);
		case USB_SEMG_GET_INIT_STATE:
			return get_init_state(filep);
		default:
			return -ENOTTY;
	}

	return retval;

}


static const struct file_operations semg_fops = {
	.owner =	THIS_MODULE,
	.read = 	semg_read,
	.write = 	semg_write,
	.open = 	semg_open,
	.flush = 	semg_flush,
	.unlocked_ioctl = semg_ioctl,
#ifdef POLL_SUPPORT
	.poll = 	semg_poll,
#endif
#ifdef ASYNC_NOTIFY_SUPPORT
	.fasync = 	semg_fasync,
#endif
	.release = 	semg_release
};


static struct usb_class_driver semg_class = {
	.name = "semg-usb%d",		/* %d为设备编号*/
	.fops = &semg_fops,
	.minor_base = USB_SEMG_MINOR_BASE
};

//TODO:
static int semg_probe(struct usb_interface *interface,
				const struct usb_device_id *id) {
	struct usb_semg *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int retval = -ENOMEM, i;
	unsigned int ep_max_size = 0;

	/* 分配设备并初始化 */
	dev = (struct usb_semg *)kzalloc(sizeof(struct usb_semg), GFP_KERNEL);
	if (!dev) {
		 dev_err(&interface->dev, "Out of memory\n");
		goto error;
	}

	/* usb_get_dev 用于内核中增加对usb_device的引用 */
	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	mutex_init(&dev->io_mutex);
	init_completion(&dev->isoc_in_completion);
	init_completion(&dev->bulk_in_completion);
	atomic_set(&dev->open_cnt, 1);			/* only can be opened once */
	kref_init(&dev->kref);
	spin_lock_init(&dev->err_lock);

	iface_desc = interface->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		endpoint = &iface_desc->endpoint[i].desc;
		if (!dev->isoc_in_endpointAddr && usb_endpoint_is_isoc_in(endpoint)) {
			ep_max_size = le16_to_cpu(endpoint->wMaxPacketSize);
			ep_max_size = (ep_max_size & 0x07ff) * (1 + ((ep_max_size >> 11) & 3));		/* 高速每小帧可传输1-4事务，虽然用不到，还是考虑进去 */

			dev->isoc_in_endpointAddr = endpoint->bEndpointAddress;
			dev->isoc_in_packetsize = ep_max_size;
			dev->isoc_in_npackets = DIV_ROUND_UP(SEMG_FRAME_SIZE, dev->isoc_in_packetsize);		/* 计算包数量即事务数，对于full speed就是帧数量 */
			dev->isoc_in_size = dev->isoc_in_packetsize * dev->isoc_in_npackets;
			/* 分配dma缓冲区，获得dma地址和cpu空间的地址 */
			dev->isoc_in_buffer = usb_alloc_coherent(dev->udev, dev->isoc_in_size,
				GFP_KERNEL, &dev->isoc_in_buffer_dma);
			if (!dev->isoc_in_buffer) {
				dev_err(&interface->dev, "Cannot allocate isoc_in_buffer");
				goto error;
			}

			/* 分配isoc的urb */
			dev->isoc_in_urb = usb_alloc_urb(dev->isoc_in_npackets/*dev->isoc_in_packetsize*/, GFP_KERNEL);
			if (!dev->isoc_in_urb) {
				dev_err(&interface->dev, "%s - Cannot allocate isoc_in_buffer", __func__);
				goto error;
			}

		} else if (!dev->bulk_in_endpointAddr && usb_endpoint_is_bulk_in(endpoint)) {
			ep_max_size = le16_to_cpu(endpoint->wMaxPacketSize);
			ep_max_size = (ep_max_size & 0x07ff) * (1 + ((ep_max_size >> 11) & 3));		/* 高速每小帧可传输1-4事务，虽然用不到，还是考虑进去 */

			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_packetsize = ep_max_size;
			dev->bulk_in_npackets = DIV_ROUND_UP(SEMG_FRAME_SIZE, dev->bulk_in_packetsize);		/* 计算包数量即事务数，对于full speed就是帧数量 */
			dev->bulk_in_size = dev->bulk_in_packetsize * dev->bulk_in_npackets;
			/* 分配dma缓冲区，获得dma地址和cpu空间的地址 */
			dev->bulk_in_buffer =usb_alloc_coherent(dev->udev, dev->bulk_in_size,
					GFP_KERNEL, &dev->bulk_in_buffer_dma);//kmalloc(dev->bulk_in_size, GFP_KERNEL);//
			if (!dev->bulk_in_buffer) {
				dev_err(&interface->dev, "Cannot allocate bulk_in_buffer");
				goto error;
			}

			/* 分配bulk的urb */
			dev->bulk_in_urb = usb_alloc_urb(dev->bulk_in_npackets/*dev->bulk_in_packetsize*/, GFP_KERNEL);
			if (!dev->bulk_in_urb) {
				dev_err(&interface->dev, "%s - Cannot allocate bulk_in_buffer", __func__);
				goto error;
			}

		}
	}

	// if (!(dev->isoc_in_endpointAddr)) {
	// 	dev_err(&interface->dev, "Cannot find isochronus enpoint");
	// 	goto error;
	// }

	if (!(dev->bulk_in_endpointAddr)) {
		dev_err(&interface->dev, "Cannot find bulk enpoint");
		goto error;
	}
	/* save semg dev pointer to this interface device */
	usb_set_intfdata(interface, dev);

	/* 注册吧，read，write都通过semg_class注册进来 */
	retval = usb_register_dev(interface, &semg_class);
	if(retval) {
		dev_err(&interface->dev, "register semg device failed");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev,
		 "USB SEMG device now attached to USBsemg-%d",
		 interface->minor);
	return 0;

error:
	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->kref, semg_delete);
	return retval;
}

/* 可能会和open竞争 */
static void semg_disconnect(struct usb_interface *interface) {
	struct usb_semg *dev;

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	/* 注销设备，归还次设备号 */
	usb_deregister_dev(interface, &semg_class);

	mutex_lock(&dev->io_mutex);
	dev->interface = NULL;
	mutex_unlock(&dev->io_mutex);


	kref_put(&dev->kref, semg_delete);
	///TODO:
    //kill all urb
	/* let the user know device disconnected */
	dev_info(&interface->dev, "USB SEMG device #%d now disconnected", interface->minor);
}



static struct usb_driver semg_driver = {
//	.owner = THIS_MODULE,
	.name = "usb_semg",
	.id_table = semg_supported_table,
	.probe = semg_probe,
	.disconnect = semg_disconnect
};

static int __init usb_semg_init(void)
{
	int result;

	/* register this driver with the USB subsystem */
	result = usb_register(&semg_driver);
	if (result)
		pr_err("usb_register failed. Error number %d", result);

	return result;
}

static void __exit usb_semg_exit(void) {
	usb_deregister(&semg_driver);
}

module_init(usb_semg_init);
module_exit(usb_semg_exit);

MODULE_AUTHOR("Yao");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0:0.1");
MODULE_DESCRIPTION("Used for producing sync signal");