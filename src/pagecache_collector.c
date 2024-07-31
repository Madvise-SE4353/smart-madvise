#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/hash.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "pagecache_collector.h"

extern pid_t target_pid_collect;
extern unsigned long start_address_collect;
extern size_t length_collect;
extern struct pid_info pid_data[HASH_SIZE];

static int handle_pre_pagefault(struct kprobe *p, struct pt_regs *regs) {
    struct vm_area_struct *vma = (struct vm_area_struct *)regs->di;
    unsigned long address = regs->si;
    unsigned int flags = regs->dx;
    u32 pid = current->pid;

    if (pid == target_pid_collect){
         int idx = hash_pid(pid);
        u64 page_addr = address >> 12; // / PAGE_SIZE) * PAGE_SIZE; //
    printk("Page Address: %llu\n: #accesse %llu", page_addr, pid_data[idx].access_count);
        // filter here 
    if ( address >= start_address_collect && address < start_address_collect + length_collect){
        //  printk(KERN_INFO "PID: %d, Target PID: %d\n", pid, target_pid);
       
        if (pid_data[idx].last_addr + 1 == page_addr) {
            pid_data[idx].access_count++;
        }

        pid_data[idx].last_addr = page_addr;
        pid_data[idx].pid = pid;
    }
    }
    printk( "handle_pre_pagefault triggered\n");

    return 0;
}

// void print_pid_data(void) {
//     int i;
//     for (i = 0; i < HASH_SIZE; i++) {
//         if  (pid_data[i].access_count != 0) {
//             printk( "PID %u: Last Addr: %llu, Access Count: %llu\n",
//                    pid_data[i].pid,
//                    (unsigned long )pid_data[i].last_addr,
//                    (unsigned long )pid_data[i].access_count);
//         }
//     }
// }