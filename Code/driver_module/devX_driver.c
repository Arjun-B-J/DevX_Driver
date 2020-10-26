#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
//#include <linux/kernel.h>

#include <linux/interrupt.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

static int __init devX_driver_init(void);
static void devX_driver_exit(void);

int devX_open (struct inode *, struct file *);
int devX_close (struct inode *, struct file *);
ssize_t devX_read (struct file *, char __user *, size_t, loff_t *);
ssize_t devX_write (struct file *, const char __user *, size_t, loff_t *);

char *memory_buffer;

struct file_operations devX_driver_fops = {
	.owner = THIS_MODULE,
	.open  = devX_open,
	.read  = devX_read,
	.write = devX_write,
	.release = devX_close
};

unsigned int i = 0;

static DECLARE_WAIT_QUEUE_HEAD(key_waitq);

static volatile int ev_press = 0;
#define IRQ_NO 1

static irqreturn_t irq_handler(int irq, void *dev_id) {
	++i;
	ev_press = 1;
	wake_up_interruptible(&key_waitq);

	printk(KERN_INFO "KEYBOARD_INTERRUPT OCCURED: %d\n",i);
	return IRQ_RETVAL(IRQ_HANDLED);
}


int devX_open (struct inode *inode, struct file *filep) {
	printk(KERN_INFO "DEVICE FILE OPENED\n");
	return 0;
}

int devX_close (struct inode *inode, struct file *filep) {
        printk(KERN_INFO "DEVICE FILE CLOSED\n");
        return 0;
}

ssize_t devX_read (struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
	
	printk(KERN_INFO "READING FROM DEVICE FILE\n");
	wait_event_interruptible(key_waitq, ev_press);
	ev_press = 0;
	copy_to_user(buffer, memory_buffer, 1);
	memset(memory_buffer,0,1);
	printk(KERN_INFO "READING COMPLETE: %c|%c\n",*memory_buffer,*buffer);
	return 0;
}

ssize_t devX_write (struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
	printk(KERN_INFO "WRITING TO DEVICE FILE\n");
	copy_from_user(memory_buffer, buffer, 1);
	printk(KERN_INFO "WRITTEN TO DEVICE FILE:%c|%c\n",*buffer,*memory_buffer);
        return len;
}


static int __init devX_driver_init(void)
{
	printk(KERN_INFO "INITIATING DEVX_DRIVER\n");

	//Register driver
	if(register_chrdev(240,"DevX Driver",&devX_driver_fops) < 0)
		return -1;
	else 
		printk(KERN_INFO "DEVX_DRIVER LOADED\n");
	//allocate memory for memory_buffer
	memory_buffer = kmalloc(1, GFP_KERNEL);
	if(!memory_buffer){
		devX_driver_exit();
		return -ENOMEM;
	}
	printk(KERN_INFO "ALLOCATED KERNEL MEMORY FOR BUFFER\n");
	memset(memory_buffer, 0, 1);

	if(request_irq(IRQ_NO, irq_handler, IRQF_SHARED, "devX", (void*)(irq_handler))) {
		printk(KERN_INFO "CANNOT REGISTER IRQ\n");
		free_irq(IRQ_NO,(void *)(irq_handler));
	}

	return 0;
}



static void devX_driver_exit(void)
{
	printk(KERN_INFO "REMOVING DEVX_DRIVER MODULE\n");
	if(memory_buffer){
		kfree(memory_buffer);
		printk(KERN_INFO "DEALLOCATED KERNEL MEMORY FOR BUFFER\n");
	}
	free_irq(IRQ_NO,(void *)(irq_handler));
	unregister_chrdev(240,"DevX Driver");
	printk(KERN_INFO "REMOVED DEVX_DRIVER MODULE\n");
}

module_init(devX_driver_init);
module_exit(devX_driver_exit);
