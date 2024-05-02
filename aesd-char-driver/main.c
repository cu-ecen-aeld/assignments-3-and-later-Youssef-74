/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>

#include "aesdchar.h"
#include "aesd_ioctl.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Youssef Essam"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */

    // get the aesd_dev structure from the c_dev pointer from inode
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry = NULL;
    size_t entry_offset = 0ULL;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    /* first check for arguments */
    if((filp == NULL) || (f_pos == NULL)) {
        return -EINVAL;
    }
    if(!access_ok(buf, count)) {
        return -EINVAL;
    }

    if(dev == NULL) {
        printk("NULL dev struct!");
        return -EINVAL;
    }

    // lock mutex
    if(mutex_lock_interruptible(&dev->lock)) 
        return -ERESTARTSYS;
    
    // find the entry offset for fpos in the aesd circular buffer
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &entry_offset); 
    if(entry == NULL) {
        PDEBUG("position exceed buffer size");
        goto out;
    }

    /* adjust count if needed */
    if(count > (entry->size  - entry_offset)) {
        count = (entry->size  - entry_offset);
    }

    if(copy_to_user(buf, entry->buffptr + entry_offset, count)){
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;
    retval = count;

    out:
        mutex_unlock(&dev->lock);
        return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev;
    const char * presult;
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    /* first check for arguments */
    if((filp == NULL) || (f_pos == NULL)) {
        return -EINVAL;
    }
    if(!access_ok(buf, count)) {
        return -EINVAL;
    }

    dev = filp->private_data;

    if(dev == NULL) {
        printk("NULL dev struct!");
        return -EINVAL;
    }

    // lock mutex
    if(mutex_lock_interruptible(&dev->lock)) 
        return -ERESTARTSYS;

    /* allocate memory for each write command as it is recived */
    // using the aesd_buffer_entry struct that defined in the aesd_dev struct to save the recived commands 
    presult = krealloc(dev->entry.buffptr, count + dev->entry.size, GFP_KERNEL); // using krealloc() help to cover the case of unterminated command
    if(presult == NULL) {
        PDEBUG("Failed to allocate memory for circular buffer entry!");
        retval = -EFAULT;
        goto out;
    }
    
    /* copy user data to buffer */
    // write starting from wrote to before start  + size
    if(copy_from_user((char*) presult + dev->entry.size, buf, count)) {
        PDEBUG("Failed to copy from user buffer!");
        retval = -EFAULT;
        goto out;
    }
    
    dev->entry.buffptr = presult;
    dev->entry.size += count;

    /* looking for \n character for terminated command */
    if(memchr(dev->entry.buffptr, '\n', dev->entry.size)) {

        // add entry to the buffer in case the command completed with terminal sign \n
        const char * replaced_buffer = aesd_circular_buffer_add_entry(&(dev->circular_buffer), &(dev->entry));

        // deallocate the replaced buffer
        kfree(replaced_buffer);
        
        // clear the entry buffer in the dev to used in the next command writting
        dev->entry.buffptr = NULL;
        dev->entry.size = 0;
    }

    *f_pos += count;
    retval = count;
    
    out:
        mutex_unlock(&dev->lock);
        return retval;
}

static loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    struct aesd_dev *dev = filp->private_data;
	loff_t newpos;
    size_t size = 0UL;
    size_t i = 0UL;
    struct aesd_buffer_entry *entry = NULL;

    /* Acquire mutex */
    if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->circular_buffer, i) 
    {
        size += entry->size;
    }

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		newpos = size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0)
        return -EINVAL;

	filp->f_pos = newpos;

    mutex_unlock(&dev->lock);

	return newpos;
}


long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0;
	int retval = 0;
    size_t size = 0;

    struct aesd_seekto seekto;
    struct aesd_dev *dev;
    
    uint8_t i;
    struct aesd_buffer_entry *entry = NULL;
    printk("Ioctl called\n");
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

    dev = filp->private_data;
    /* Acquire mutex */
    if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

    if(arg == 0)
    {
        return -EFAULT;
    }
    else
    {
        retval = copy_from_user(&seekto, (void *)arg, sizeof(struct aesd_seekto));
        if(retval)
        {
            retval = -EFAULT;
            goto leave;
        }
    }

	if(cmd == AESDCHAR_IOCSEEKTO)
    {
        if(seekto.write_cmd > 9)
            return -EINVAL;
        
        if(seekto.write_cmd_offset >= dev->circular_buffer.entry[seekto.write_cmd].size)
            return -EINVAL;

        AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, i) 
        {
            if(i < seekto.write_cmd)
            {
                size += entry->size;
            }
            else
            {
                size += seekto.write_cmd_offset;
                break;
            }
        }

        filp->f_pos = size;  
        //printk("Fpos %d\n", size);
    }

leave:
    mutex_unlock(&dev->lock);
	return retval;

}


struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_ioctl
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    
    aesd_circular_buffer_init(&aesd_device.circular_buffer);
    mutex_init(&aesd_device.lock);
    printk("Mutex initialized\n");
    
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    uint8_t i;
    struct aesd_buffer_entry *entry = NULL;
    cdev_del(&aesd_device.cdev);

    printk("Deleted cdev\n");
    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, i) 
    {
        kfree(entry->buffptr);
    }

    printk("Freed buffers \n");  
    mutex_destroy(&aesd_device.lock);
    printk("Destroyed mutexss \n");
    unregister_chrdev_region(devno, 1);
}
 


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
