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

//TODO
#define USB_SEMG_VENDOR_ID 
#define USB_SEMG_PRODUCT_ID

#define SEMG_FRAME_SIZE 		3257
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
	struct usb_interface 	*interface  /* usb interface 的引用*/
	__u8            isoc_in_endpointAddr;
	size_t 			isoc_in_size;		/* the size of the receive buffer */
	size_t			isoc_in_filled;		/* number of bytes in the buffer */
	size_t			isoc_in_copied;		/* already copied to user space */
	size_t 			isoc_in_packetsize;		/* Max Packet size of isoc endpoint */
	size_t 			isoc_in_npackets;	/* number of packets */
	void 	*isoc_in_buffer;
	void 	*isoc_in_buffer_dma;
	struct urb 		*isoc_in_urb;		/* 用来读取数据的urb*/
	struct completion 	isoc_in_completion; 	/* completion for read */
	struct kref 	kref;				/* 设备引用计数 */
	//atomic_t /* 跟踪打开计数 */
	struct mutex 	io_mutex; 			/* 用于读写，断开之间的同步 */
};
#define to_semg_dev(p) container(p, struct usb_semg, kref)

static struct usb_driver semg_driver;//static类型的前向声明，长见识了吧。因为后面也有定义并初始化了	

static const struct file_operations semg_fops = {
	.owner =	THIS_MODULE,
	.read = 	semg_read,
	.write = 	semg_write,
	.open = 	semg_open,
	.release = 	semg_release,
	.flush = 	semg_flush
};

static void semg_read_isoc_callback(struct urb *urb)
{
	struct usb_semg *dev = urb->context;
	complete(&dev->isoc_in_completion);
}

/*
 * Block Read
 */
static ssize_t semg_read(struct file *file, char __user *user_buf, size_t count, loff_t *pos) {

	struct usb_semg *dev = (struct usb_semg *)file->private_data;
	struct urb  *urb = dev->urb;
	unsigned int i;
	int retval = 0;
	
	//dev->isoc_in_buffer = usb_alloc_coherent(dev->udev, )

	/* 初始化urb */
	urb->dev = dev->udev;
	urb->pipe = usb_recvisocpipe(dev->udev, dev->isoc_in_endpointAddr);
	urb->interval = 1;///TODO:not sure//yes sure
	urb->context = dev;
	urb->complete = semg_read_isoc_callback;
	urb->transfer_buffer = dev->isoc_in_buffer;		 
	urb->transfer_dma = dev->isoc_in_buffer_dma;
	urb->number_of_packets = dev->isoc_in_npackets 
	urb->transfer_buffer_length = dev->isoc_in_size;      //TODO   是不是该使用SEMG_FRAME_SIZE;
	urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;//尽早开始同步传输，优选使用DMA传输
	/* 设置isoc各帧的缓冲区位移，每帧预计长度 */
	for (i = 0; i < urb->number_of_packets; i++) {
		urb->iso_frame_desc[i].offset = i * dev->isoc_in_packetsize;
		urb->iso_frame_desc[i].length = dev->isoc_in_packetsize;
	}

	/* submit urb */
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if (retval < 0) {
		err("Failed to submit in urb");
		goto out;
	}

	/*wait for read operation complete*/
	wait_for_completion(&dev->isoc_in_completion);
	//TODO
	if((dev->isoc_in_filled != SEMG_FRAME_SIZE) || (SEMG_FRAME_SIZE != count) {
		retval = -1;
		goto out;
	}

	if(copy_to_user(user_buf, dev->isoc_in_buffer, count)) {
		retval = -EFAULT;
		goto out;
	}

	retval = count;

out:
	return retval;
}

static ssize_t semg_write(struct file *file, const char __user *uesr_buf, size_t count, loff_t *f_pos) {


}
static int semg_open(struct inode *inode, struct file *file) {
	struct usb_interface *interface;
	struct usb_semg *dev;
	int retval = 0;
	int subminor;

	subminor = iminor(inode);//获取次设备号
	interface = usb_find_interface(&semg_driver, subminor)；
	if (!inteface) {				//可能usb设备刚被拔了
		err("%s - error, can't find semg usb device for minor %d", __func__, subminor);
		retval = -ENODEV;
		goto error;
	}

	dev = usb_get_infdata(interface); //可能usb设备刚被拔了
	if(!dev) {
		retval = -ENODEV;
		goto error;
	}

	if(!atomic_dec_and_test(&dev->open_cnt)) {
		atomic_inc(&dev->open_cnt);
		return -EBUSY;		/* 已经被打开过了 */
	}

	mutex_lock(&dev->io_mutex);
	retval = usb_autopm_get_interface(interface);	/* 阻止usb设备autosuspended */
	if (retval) {
		mutex_unlock(&dev->io_mutex);
		kref_put(&dev->kref, skel_delete);
		goto exit;
	}

	file->private_data = dev;  //save dev for other functions use
	mutex_unlock(&dev->io_mutex);

	kref_get(&dev->kref);//引用加1





error:
	 
	return retval;
}

static int semg_release(struct inode *inode, struct file *file) {
	struct usb_semg *dev;

	dev = file->private_data;
	if (dev == NULL)	//怎么可能？
		return -ENODEV;

	atomic_inc(&dev->open_cnt);
	mutex_lock(&dev->io_mutex);
	if(dev->interface)
		usb_autopm_put_interface(dev->interface);		/* 允许usb设备autosuspended */
	mutex_unlock(&dev->io_mutex);
	
	/* decrement the count on our device */
	kref_put(&dev->kref, skel_delete);
	return 0;
}

static int semg_flush(struct file *file, fl_owner_t id) {

}

static struct usb_class_driver semg_class = {
	.name = "semg%d",		/* %d为设备编号*/
	.fops = &semg_fops,
	.minor_base = USB_SEMG_MINOR_BASE
};



//TODO:
static int semg_probe(struct_interface *inf, 
				const struct usb_device_id *id) {
	struct usb_semg *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int retval = 0;
	unsigned int ep_max_size = 0;

	/* 分配设备并初始化 */
	dev = (struct usb_semg *)kzalloc(sizeof(struct usb_semg), GFP_KERNEL);
	if (!dev) {
		err("Out of Memory");
		goto error;
	}

	/* usb_get_dev 用于内核中增加对usb_device的引用 */
	dev->udev = usb_get_dev(interface_to_usbdev(inf));
	dev->interface = inf;

	mutex_init(&dev->io_mutex);
	init_completion(&dev->isoc_in_completion);
	atominc_set(&dev->open_cnt, 1);			/* only can be opened once */ 
	kref_init(&dev->kref);

	iface_desc = interface->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoint; i++) {
		endpoint = iface_desc->endpoint[i].desc;
		if (!dev->isoc_in_endpointAddr && usb_endpoint_is_isoc_in(endpoint)) {
			ep_max_size = le16_to_cpu(endpoint->wMaxPacketSize);
			ep_max_size = (ep_max_size & 0x07ff) * (1 + ((ep_max_size >> 11) & 3));		/* 高速每小帧可传输1-4事务，虽然用不到，还是考虑进去 */

			dev->isoc_in_endpointAddr = enpoint->bEndpointAddress;
			dev->isoc_in_packetsize = ep_max_size;
			dev->isoc_in_npackets = DIV_ROUND_UP(SEMG_FRAME_SIZE, isoc_in_packetsize);		/* 计算包数量即事务数，对于full speed就是帧数量 */
			dev->isoc_in_size = dev->isoc_in_packetsize * dev->isoc_in_npackets;
			/* 分配dma缓冲区，获得dma地址和cpu空间的地址 */
			dev->isoc_in_buffer = usb_buffer_alloc(dev->udev, dev->isoc_in_size,
				GFP_KERNEL, &dev->isoc_in_buffer_dma);
			if (!dev->isoc_in_buffer) {
				err("Cannot allocate isoc_in_buffer");
				goto error;
			}

			/* 分配isoc的urb */
			dev->isoc_in_urb = usb_alloc_urb(dev->isoc_in_packetsize, GFP_KERNEL);
			if (!dev->isoc_in_urb) {
				err("%s - Cannot allocate isoc_in_buffer", __func__);
				goto error;
			}

		}
	}
	if (!(dev->isoc_in_endpointAddr)) {
		err("Cannot find isochronus enpoint");
		goto error;
	}

	/* save semg dev pointer to this interface device */
	usb_set_infdata(interface, dev);

	/* 注册吧，read，write都通过semg_class注册进来 */
	retval = usb_regiter_dev(interface, &semg_class);
	if(retval) {
		err("register semg device failed");
		usb_set_infdata(interface, NULL);
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
}

/* 可能会和open竞争 */
static void semg_disconnect(struct usb_interface *interface) {
	struct usb_semg *dev;

	dev = usb_get_infdata(interface);
	usb_set_infdata(interface, NULL);

	/* 注销设备，归还次设备号 */
	usb_deregister_dev(interface, &semg_class);

	mutex_lock(&dev->io_mutex);
	dev->interface = NULL;
	mutex_unlock(&dev->io_mutex);


	kref_put(&dev->kref, smeg_delte)
	///TODO:
    kill all urb
	/* let the user know device disconnected */
	dev_info(&interface->dev, "USB SEMG device #%d now disconnected", interface->minor);
}

static void semg_delete(struct kref *kref) {
	struct usb_semg *dev = to_semg_dev(kref);

	usb_free_urb(dev->isoc_in_urb);
	usb_put_dev(dev->udev);
	free_page(dev->isoc_in_buffer);
}

static struct usb_driver semg_driver = {
	.owner = THIS_MODULE,
	.name = "semg_driver",
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
		err("usb_register failed. Error number %d", result);

	return result;
}

static void __exit usb_semg_exit(void) {
	usb_deregister(&semg_driver)
}

module_init(usb_semg_init)
module_exit(usb_semg_exit)

MODULE_AUTHOR("Yao");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0:0.1");
MODULE_DESCRIPTION("Used for producing sync signal");
