#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");

int Hello_Var = 0;
module_param(Hello_Var, int, S_IRUGO);

EXPORT_SYMBOL_GPL(Hello_Var);

dev_t devno;
struct class *HelloDev_class;
struct cdev  HelloDev_cdev;

char 		tampon[10] = {0,0,0,0,0,0,0,0,0,0};
uint16_t num = 0;

int HelloDev_open(struct inode *inode, struct file *filp) {
	printk(KERN_WARNING"HelloDev_open (%s:%u)\n", __FUNCTION__, __LINE__);
	return 0;
}

int HelloDev_release(struct inode *inode, struct file *filp) {
	printk(KERN_WARNING"HelloDev_release (%s:%u)\n", __FUNCTION__, __LINE__);
	return 0;
}

static ssize_t HelloDev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	char ch;

	if (num > 0) {
		ch = tampon[--num];
		copy_to_user(buf, &ch, 1);
		printk(KERN_WARNING"HelloDev_read (%s:%u) count = %lu ch = %c\n", __FUNCTION__, __LINE__, count, ch);
		return 1;
	} else {
		printk(KERN_WARNING"HelloDev_read (%s:%u) count = %lu ch = no char\n", __FUNCTION__, __LINE__, count);
		return 0;
	}
}

static ssize_t HelloDev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	char ch;

	if (num < 10) {
		copy_from_user(&ch, buf, 1);
		tampon[num++] = ch;
		printk(KERN_WARNING"HelloDev_write (%s:%u) count = %lu ch = %c\n", __FUNCTION__, __LINE__, count, ch);
		return 1;
	} else {
		printk(KERN_WARNING"HelloDev_write (%s:%u) count = %lu ch = no place\n", __FUNCTION__, __LINE__, count);
		return -EAGAIN;
	}
}

struct file_operations HelloDev_fops = {
	.owner	=	THIS_MODULE,
	.read		=	HelloDev_read,
	.write	=	HelloDev_write,
	.open		=	HelloDev_open,
	.release	=	HelloDev_release
};


static int __init hellodev_init (void) {
	int result;

	printk(KERN_ALERT"HelloDev_init (%s:%u) => Hello, World !!!\n", __FUNCTION__, __LINE__);

	result = alloc_chrdev_region(&devno, 0, 1, "MyHelloDev");
	if (result < 0)
		printk(KERN_WARNING"HelloDev_init ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
	else
		printk(KERN_WARNING"HelloDev_init : MAJOR = %u MINOR = %u (Hello_Var = %u)\n", MAJOR(devno), MINOR(devno), Hello_Var);

	HelloDev_class = class_create(THIS_MODULE, "HelloDevClass");
	device_create(HelloDev_class, NULL, devno, NULL, "HelloDev_Node");
	cdev_init(&HelloDev_cdev, &HelloDev_fops);
	HelloDev_cdev.owner = THIS_MODULE;
	if (cdev_add(&HelloDev_cdev, devno, 1) < 0)
		printk(KERN_WARNING"HelloDev ERROR IN cdev_add (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

	return 0;
}

static void __exit hellodev_exit (void) {
	cdev_del(&HelloDev_cdev);
	unregister_chrdev_region(devno, 1);
	device_destroy (HelloDev_class, devno);
	class_destroy(HelloDev_class);

	printk(KERN_ALERT"HelloDev_exit (%s:%u) => Goodbye, cruel world\n", __FUNCTION__, __LINE__);
}

module_init(hellodev_init);
module_exit(hellodev_exit);


// make -C /usr/src/linux-source-3.16.0 M=`pwd` modules
// sudo insmod ./HelloDev.ko
// sudo chmod 666 /dev/HelloDev_Node
// sudo rmmod HelloDev
// lsmod | grep HelloDev
// ls -la /dev | grep HelloDev
// cat /proc/devices
// ls -la /sys/module/Hello*
// ls -la /sys/devices/virtual | grep Hello
// ls -la /sys/devices/virtual/HelloDevClass
// ls -la /sys/devices/virtual/HelloDevClass/HelloDev_Node

// dmesg | tail -20
// watch -n 0,1 "dmesg | grep HelloDev | tail -n $((LINES-6))"

