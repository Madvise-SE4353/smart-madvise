#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/ptrace.h>
#include <linux/socket.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include "smart_madvise_ioctl.h"
#include "executor.h"
#include "global_map.h"
#include "daemon.h"

MODULE_AUTHOR("Smart Madvise Group");
MODULE_DESCRIPTION("Smart Madvise");
MODULE_LICENSE("GPL");

// ioctl part
static int madvise_cnt;
static struct cdev ioctl_demo_cdev;
static int ioctl_demo_major;
static struct class *cls;
static const unsigned int num_of_dev = 1; // const?

// sys_call_table part
typedef int (*syscall_wrapper)(struct pt_regs *);
unsigned long sys_call_table_addr;
syscall_wrapper original_madvise;

// other global variables
global_task_map task_map_global;
pid_t register_pid;

unsigned long kaddr_lookup_name(const char *fname_raw);
static int sys_madvise_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs);

struct kprobe syscall_kprobe = {
    .symbol_name = "__x64_sys_madvise",
    .pre_handler = sys_madvise_kprobe_pre_handler,
};

struct kprobe schdule_kprobe = {
    .symbol_name = "schedule",
    .post_handler = schedule_post_handler,
};

unsigned long kaddr_lookup_name(const char *fname_raw)
{
    int i;
    unsigned long kaddr;
    char *fname_lookup, *fname;

    fname_lookup = kvzalloc(NAME_MAX, GFP_KERNEL);
    if (!fname_lookup)
        return 0;

    fname = kvzalloc(NAME_MAX, GFP_KERNEL);
    if (!fname)
        return 0;

    /*
     * We have to add "+0x0" to the end of our function name
     * because that's the format that sprint_symbol() returns
     * to us. If we don't do this, then our search can stop
     * prematurely and give us the wrong function address!
     */
    strcpy(fname, fname_raw);
    strcat(fname, "+0x0");
    printk("fname = %s\n", fname);

    /*
     * Get the kernel base address:
     * sprint_symbol() is less than 0x100000 from the start of the kernel, so
     * we can just AND-out the last 3 bytes from it's address to the the base
     * address.
     * There might be a better symbol-name to use?
     */
    kaddr = (unsigned long)&sprint_symbol;
    kaddr &= 0xffffffffff000000;

    /*
     * All the syscalls (and all interesting kernel functions I've seen so far)
     * are within the first 0x100000 bytes of the base address. However, the kernel
     * functions are all aligned so that the final nibble is 0x0, so we only
     * have to check every 16th address.
     */
    for (i = 0x0; i < 0x200000; i++)
    {
        /*
         * Lookup the name ascribed to the current kernel address
         */
        sprint_symbol(fname_lookup, kaddr);

        /*
         * Compare the looked-up name to the one we want
         */
        if (strncmp(fname_lookup, fname, strlen(fname)) == 0)
        {
            /*
             * Clean up and return the found address
             */
            sprint_symbol(fname_lookup, kaddr);
            printk("sys_call_table name: %s\n", fname_lookup);

            kvfree(fname_lookup);
            return kaddr;
        }
        /*
         * Jump 16 addresses to next possible address
         */
        kaddr += 0x10;
    }
    /*
     * We didn't find the name, so clean up and return 0
     */
    kvfree(fname_lookup);
    return 0;
}

static int sys_madvise_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    // struct pt_regs *madvise_regs = (struct pt_regs *)(regs->di);
    // printk("do_madvise kprobe\n");
    // printk("process %d call customized madvise, rip 0x%lX and 0x%lX, addr 0x%lX ,di 0x%lX, si 0x%lX, dx %lu\n",current->pid ,regs->ip,madvise_regs->ip,(unsigned long)(p->addr), madvise_regs->di, madvise_regs->si, madvise_regs->dx);

    // we can manually call the madvise system call by the function point we get from sys_call_table
    // although sys_call_table isn't used in new linux kernel version, the data still exists in kernel memory space and has correct function points
    // printk("return value: %d\n",original_madvise(madvise_regs));

    // we can modify the first argument of subsequent madvise system call so it will can't have an effect on memory
    // madvise_regs->di = 1;
    madvise_cnt += 1;

    return 0;
}

static long
smart_madvise_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    // struct ioctl_demo_data *ioctl_data = &ioctl_demo_static_data;
    // struct ioctl_demo_data *ioctl_data = filp->private_data;
    int retval = 0;
    // int val;
    struct ioctl_demo_arg data;
    memset(&data, 0, sizeof(data));
    switch (cmd)
    {
    case IOCTL_DEMO_VALGET_NUM:
    {
        retval = put_user(madvise_cnt, (int __user *)arg);
        break;
    }

    case IOCTL_DEMO_VALSET_NUM:
    {
        retval = get_user(madvise_cnt, (int __user *)arg);
        break;
    }
    case IOCTL_DEMO_REGISTER_NUM:
    {
        pid_t new_pid = current->pid;
        printk("register pid %d\n", new_pid);
        register_pid = new_pid;
        smart_madvise_ioctl_args obj;
        unsigned long copied = copy_from_user(&obj, (const void __user *)arg, sizeof(smart_madvise_ioctl_args));
        if (copied)
        {
            printk(KERN_WARNING "Failed to copy %lu bytes to kernel space.\n", copied);
            retval = -ENOTTY;
            break;
        }
        printk("start 0x%lX, length %ld\n", obj.start, obj.len);
        add_task(&task_map_global, new_pid, obj.start, obj.len, SMART_MADVISE_TASK_MADVISE, 1);
        break;
    }
    case IOCTL_DEMO_UNREGISTER_NUM:
    {
        printk("unregister pid %d\n", register_pid);
        register_pid = 0;
        break;
    }
    default:
    {
        retval = -ENOTTY;
        break;
    }
    }
    return retval;
}

static int smart_madvise_open(struct inode *inode, struct file *filp)
{
    // struct ioctl_demo_data *ioctl_data;

    pr_alert("%s call.\n", __func__);
    // ioctl_data = kmalloc(sizeof(struct ioctl_demo_data), GFP_KERNEL);

    // if (ioctl_data == NULL)
    //     return -ENOMEM;

    // rwlock_init(&ioctl_data->lock);
    // ioctl_data->val = 0xFF;
    // filp->private_data = ioctl_data;

    return 0;
}

static int smart_madvise_close(struct inode *inode, struct file *filp)
{
    pr_alert("%s call.\n", __func__);

    // if (filp->private_data) {
    //     kfree(filp->private_data);
    //     filp->private_data = NULL;
    // }

    return 0;
}

static struct file_operations fops = { 
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0) 
    .owner = THIS_MODULE, 
#endif 
    .open = smart_madvise_open, 
    .release = smart_madvise_close, 
    // .read = test_ioctl_read, 
    .unlocked_ioctl = smart_madvise_ioctl, 
}; 

static int smart_madvise_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init my_module_init(void)
{
    int err;
    dev_t dev;
    int alloc_ret = -1;
    int cdev_ret = -1;
    printk("my_module_init\n");

    // init some global variables
    register_pid = 0;

    // init task map work
    init_task_map(&task_map_global);

    // init kprobe work
    err = register_kprobe(&syscall_kprobe);
    if (err)
    {
        pr_err("register_kprobe() madivse failed: %d\n", err);
        return err;
    }
    err = register_kprobe(&schdule_kprobe);
    if (err)
    {
        pr_err("register_kprobe() kprobe failed: %d\n", err);
        return err;
    }

    // init ioctl work
    alloc_ret = alloc_chrdev_region(&dev, 0, num_of_dev, IOCTL_DEMO_DRIVER_NAME);
    if (alloc_ret)
        goto error;
    ioctl_demo_major = MAJOR(dev);
    cdev_init(&ioctl_demo_cdev, &fops);
    cdev_ret = cdev_add(&ioctl_demo_cdev, dev, num_of_dev);
    pr_alert("%s driver(major: %d) installed.\n", IOCTL_DEMO_DRIVER_NAME,
             ioctl_demo_major);
    cls = class_create(IOCTL_DEMO_DEVICE_FILE_NAME);
    cls->dev_uevent = smart_madvise_uevent;
    device_create(cls, NULL, dev, NULL, IOCTL_DEMO_DEVICE_FILE_NAME);
    pr_info("Device created on /dev/%s\n", IOCTL_DEMO_DEVICE_FILE_NAME);

    // init sys_call_table work
    sys_call_table_addr = kaddr_lookup_name("sys_call_table");
    printk("sys_call_table@%lx\n", sys_call_table_addr);
    original_madvise = ((syscall_wrapper *)sys_call_table_addr)[__NR_madvise];
    printk("original_madvise = %p\n", original_madvise);

    if (smart_madvise_start_daemon() != 0) {
        goto error;
    }

    return 0;
error:
    if (cdev_ret == 0)
        cdev_del(&ioctl_demo_cdev);
    if (alloc_ret == 0)
        unregister_chrdev_region(dev, num_of_dev);
    return -1;
}

static void __exit my_module_exit(void)
{
    printk("my_module_exit\n");

    // kprobe work
    unregister_kprobe(&syscall_kprobe);
    unregister_kprobe(&schdule_kprobe);

    // ioctl work
    dev_t dev = MKDEV(ioctl_demo_major, 0);
    device_destroy(cls, dev);
    class_destroy(cls);
    cdev_del(&ioctl_demo_cdev);
    unregister_chrdev_region(dev, num_of_dev);

    smart_madvise_stop_daemon();
}

module_init(my_module_init);
module_exit(my_module_exit);