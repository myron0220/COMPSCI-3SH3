/*
* macid: li64; avenue name: Henry Li
* macid: wangm235; avenue name: Mingzhe Wang
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#define PROC_NAME "jiffies"

struct proc_dir_entry *jiffies_proc;

/* This function is called when /proc/jiffies is read. */
ssize_t proc_read(struct file *filp, char *usr_buf, size_t count, loff_t *offp)
{
    int result = 0;
    char buffer[64];
    unsigned long jiffies_toshow;
    
    // exit after the jiffie is read
    static int finish = 0;
    if (finish) {
        finish = 0;
        return 0;
    }
    finish = 1;
    
    jiffies_toshow = jiffies;
    result = sprintf(buffer, "%lu\n", jiffies_toshow);
    printk(KERN_INFO "Successful Write %lu To: /proc/jiffies\n", jiffies_toshow);
    // copies kernel buffer to user buffer
    copy_to_user(usr_buf, buffer, result);
    return result;
}

/* File operation on proc */
 struct file_operations proc_fops = {
    .owner=THIS_MODULE,
    .read=proc_read,
};

/* This function is called when the module is loaded. */
int jiffies_init(void)
{
    printk(KERN_INFO "Loading Module\n");
	// creates /proc/jiffies
    jiffies_proc = proc_create(PROC_NAME, 0, NULL, &proc_fops);
	printk(KERN_INFO "jiffies = %lu\n",jiffies);
	printk(KERN_INFO "/proc/jiffies created\n");
    return 0;
}

/* This function is called when the module is removed. */
void jiffies_exit(void) {
	printk(KERN_INFO "Removing Module\n");
    printk(KERN_INFO "/proc/jiffies removed\n");
    remove_proc_entry(PROC_NAME, NULL);
}


/* Macros for registering module entry and exit points. */
module_init( jiffies_init );
module_exit( jiffies_exit );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Jiffies Module");
MODULE_AUTHOR("Henry");

