#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#define UAF_LKM_ALLOC _IO('F', 0)
#define UAF_LKM_FREE  _IO('F', 1)
#define UAF_LKM_USE   _IO('F', 2)

struct uaf_lkm_victim
{
    char msg_msg[48];
    unsigned long flag;
    char fill[968];
};
struct uaf_lkm_info
{
	struct uaf_lkm_victim* victim; 
	bool freed;
};

static long ioctl_handler(struct file* fdevice, unsigned int cmd, unsigned long arg)
{
	struct uaf_lkm_info* info = fdevice->private_data;
	
	switch (cmd)
	{
		case UAF_LKM_ALLOC:
		if (info->freed)
		{
			struct uaf_lkm_victim* victim = (struct uaf_lkm_victim*) kmalloc(sizeof(struct uaf_lkm_victim), GFP_KERNEL);
			info->victim = victim;
			info->freed = false;
			victim->flag = 0x4141414141414141;
			return 0;
		}
		return EEXIST;
		
		case UAF_LKM_FREE:
		if (!info->freed)
		{
			info->freed = true;
			kfree(info->victim);
			return 0;
		}
		return EAGAIN;
		
		case UAF_LKM_USE:	
		if (info->victim->flag == 0x4141414141414141)
		{
			printk(KERN_INFO "call valid function \n");
			return EACCES;
		} 
		else if (info->victim->flag == 0xdeadbeafdeadbeaf)
		{
			printk(KERN_INFO "call hack function \n");
			return 0;
		}
		
		printk(KERN_INFO "fault -> %lx \n", info->victim->flag);
		return EFAULT;
	}
	
	return ENOTSUPP;
}

static int uaf_lkm_release(struct inode* inode, struct file* fdevice)
{
	struct uaf_lkm_info* info = fdevice->private_data;
	
	if (!info->freed)
	{
		kfree(info->victim);
	}
	
	kfree(info);
	return 0;
}

static int uaf_lkm_open(struct inode* inode, struct file* fdevice)
{
	struct uaf_lkm_info* info = (struct uaf_lkm_info*) kmalloc(sizeof(struct uaf_lkm_info), GFP_KERNEL);
	info->victim = NULL;
	info->freed = true;
	fdevice->private_data = info;
	return 0;
}

struct file_operations uaf_fops = {
	.open = uaf_lkm_open,
	.owner = THIS_MODULE,
	.unlocked_ioctl = ioctl_handler,
	.release = uaf_lkm_release
};

struct miscdevice uaf_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "uaf_lkm",
	.fops  = &uaf_fops,
	.mode  = 0777
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yurii Crimson");
MODULE_DESCRIPTION("Test UAF kernel module");
MODULE_VERSION("0.01");

static int __init lkm_init(void) {
	printk(KERN_INFO "Init lkm uaf \n");
	misc_register(&uaf_device);
	return 0;
}

static void __exit lkm_exit(void) {
	printk(KERN_INFO "Unload lkm uaf \n");
	misc_deregister(&uaf_device);
}

module_init(lkm_init);
module_exit(lkm_exit);

