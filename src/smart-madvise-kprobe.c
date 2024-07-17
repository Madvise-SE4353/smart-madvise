#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/ptrace.h>
#include <linux/socket.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

#include "myprintk.h"

MODULE_AUTHOR("Haocheng Wang");
MODULE_DESCRIPTION("Smart Madvise");
MODULE_LICENSE("GPL");

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

typedef int (*syscall_wrapper)(struct pt_regs *);
unsigned long sys_call_table_addr;
syscall_wrapper original_madvise;


static int sys_madvise_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct pt_regs *madvise_regs = (struct pt_regs *)(regs->di);
    myprintk();
    printk("process %d call customized madvise, rip 0x%lX and 0x%lX, addr 0x%lX ,di 0x%lX, si 0x%lX, dx %lu\n",current->pid ,regs->ip,madvise_regs->ip,(unsigned long)(p->addr), madvise_regs->di, madvise_regs->si, madvise_regs->dx);
    
    // we can manually call the madvise system call by the function point we get from sys_call_table
    // although sys_call_table isn't used in new linux kernel version, the data still exists in kernel memory space and has correct function points 
    printk("return value: %d\n",original_madvise(madvise_regs));

    // we can modify the first argument of subsequent madvise system call so it will can't have an effect on memory   
    madvise_regs->di = 1;
    
    return 0;
}

struct kprobe syscall_kprobe = {
    .symbol_name = "__x64_sys_madvise",
    .pre_handler = sys_madvise_kprobe_pre_handler,
};

static int __init my_module_init(void)
{
    int err;

    printk("my_module_init\n");
    err = register_kprobe(&syscall_kprobe);
    if (err)
    {
        pr_err("register_kprobe() failed: %d\n", err);
        return err;
    }


    sys_call_table_addr = kaddr_lookup_name("sys_call_table");
    printk("sys_call_table@%lx\n", sys_call_table_addr);
    original_madvise = ((syscall_wrapper *)sys_call_table_addr)[__NR_madvise];
    printk("original_madvise = %p\n", original_madvise);

    return 0;
}

static void __exit my_module_exit(void)
{
    printk("my_module_exit\n");
    unregister_kprobe(&syscall_kprobe);
}

module_init(my_module_init);
module_exit(my_module_exit);