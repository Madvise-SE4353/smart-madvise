#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/ptrace.h>
#include <linux/socket.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

MODULE_AUTHOR("Haocheng Wang");
MODULE_DESCRIPTION("Customized Madvise");
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

#define SOCKETLOG "[SOCKETLOG]"

int enable_page_rw(void *ptr)
{
    unsigned int level;
    pte_t *pte = lookup_address((unsigned long)ptr, &level);
    if (pte->pte & ~_PAGE_RW)
    {
        pte->pte |= _PAGE_RW;
    }
    return 0;
}

int disable_page_rw(void *ptr)
{
    unsigned int level;
    pte_t *pte = lookup_address((unsigned long)ptr, &level);
    pte->pte = pte->pte & ~_PAGE_RW;
    return 0;
}

syscall_wrapper original_madvise;

// asmlinkage int log_madvise(int sockfd, const struct sockaddr *addr, int addrlen) {
int log_madvise(struct pt_regs *regs)
{
    printk("customized madvise was called\n");
    return (*original_madvise)(regs);
}

static int __init start(void)
{

    printk("customized madvise module has been loaded\n");

    sys_call_table_addr = kaddr_lookup_name("sys_call_table");

    printk("sys_call_table@%lx\n", sys_call_table_addr);

    enable_page_rw((void *)sys_call_table_addr);
    original_madvise = ((syscall_wrapper *)sys_call_table_addr)[__NR_madvise];
    if (!original_madvise)
    {
        printk("original_madvise find error!\n");
        return -1;
    }
    ((syscall_wrapper *)sys_call_table_addr)[__NR_madvise] = log_madvise;
    disable_page_rw((void *)sys_call_table_addr);

    printk("original_madvise = %p\n", original_madvise);
    printk("customized_madvise = %p\n", (void *)&log_madvise);
    return 0;
}

static void __exit stop(void)
{
    printk("customized madvise module has been unloaded\n");

    enable_page_rw((void *)sys_call_table_addr);
    ((syscall_wrapper *)sys_call_table_addr)[__NR_madvise] = original_madvise;
    disable_page_rw((void *)sys_call_table_addr);
}

module_init(start);
module_exit(stop);