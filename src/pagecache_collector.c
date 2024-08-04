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
#include "global_map.h"

// change stat threshold
#define SEQ_THRESHOLD 100
// extern pid_t target_pid_collect;
// extern unsigned long start_address_collect;
// extern size_t length_collect;
extern struct pid_info pid_data[HASH_SIZE];

int hash_pid(u32 pid)
{
    return hash_32(pid, 8) % HASH_SIZE;
}

int handle_pre_pagefault(struct kprobe *p, struct pt_regs *regs) {
    struct vm_area_struct *vma = (struct vm_area_struct *)regs->di;
    unsigned long address = regs->si;
    unsigned int flags = regs->dx;
    u32 pid = current->pid;
    int idx = hash_pid(pid);
    struct pid_info *current_pid_info = &pid_data[idx];

    if (current_pid_info->pid != pid){
        return 0;
    }

    if (current_pid_info->tracked) {
        unsigned long page_addr = address >> 16;
        // if (current_pid_info->last_addr + 1 == page_addr) {
        //     current_pid_info->seq_access_count++;
        // }
        // .. print start address and len
        if ( address >= current_pid_info->start_address_collect 
          && address < current_pid_info->start_address_collect + current_pid_info->length_collect){
                    // printk("address: %lx, page_addr: %lx\n", address, page_addr);
            if (current_pid_info->last_addr + 1 == page_addr) {

                current_pid_info->seq_access_count++;
                // if (current_pid_info->seq_access_count !=1)
                //     pr_info("seq counter: %d\n", current_pid_info->seq_access_count);
            }
              else{
            current_pid_info->rand_access_count++;
        }
            // pr_info("smart-madvise-collector: tracked process and address, pid: %d, counter: %d\n", pid, current_pid_info->seq_access_count);
            // DO STH HERE
            switch(current_pid_info->state) {
                case SMART_MADVISE_STATE_INITIAL:
                    if(current_pid_info->seq_access_count > current_pid_info->rand_access_count + SEQ_THRESHOLD){
                        pr_info("smart-madvise: switching to MADV_SEQUENTIAL, deregistering PID %d\n", pid);
                        add_task(&task_map_global, current_pid_info->pid, current_pid_info->start_address_collect, current_pid_info->length_collect, SMART_MADVISE_TASK_MADVISE, SMART_MADVISE_SEQUENTIAL);
                        current_pid_info->state = SMART_MADVISE_STATE_SEQUENTIAL;
                    }
                    else if (current_pid_info->rand_access_count > current_pid_info->seq_access_count + SEQ_THRESHOLD){
                        pr_info("smart-madvise: switching to MADV_RANDOM, deregistering PID %d\n", pid);
                        add_task(&task_map_global, current_pid_info->pid, current_pid_info->start_address_collect, current_pid_info->length_collect, SMART_MADVISE_TASK_MADVISE, SMART_MADVISE_RANDOM);
                        current_pid_info->state = SMART_MADVISE_STATE_RANDOM;
                    }
                    break;
                case SMART_MADVISE_STATE_SEQUENTIAL:
                       // reseting the counter to not make it too big 
                // same as not tracking the state if we are in it
                current_pid_info->seq_access_count = 0;
                if(current_pid_info->rand_access_count > current_pid_info->seq_access_count + SEQ_THRESHOLD){
                        pr_info("smart-madvise: switching to MADV_RANDOM, deregistering PID %d\n", pid);
                        add_task(&task_map_global, current_pid_info->pid, current_pid_info->start_address_collect, current_pid_info->length_collect, SMART_MADVISE_TASK_MADVISE, SMART_MADVISE_RANDOM);
                        current_pid_info->state = SMART_MADVISE_STATE_RANDOM;
                    }
                    break;
                case SMART_MADVISE_STATE_RANDOM:
                current_pid_info->rand_access_count = 0;
                    if(current_pid_info->seq_access_count > current_pid_info->rand_access_count + SEQ_THRESHOLD){
                        pr_info("smart-madvise: switching to MADV_SEQUENTIAL, deregistering PID %d\n", pid);
                        add_task(&task_map_global, current_pid_info->pid, current_pid_info->start_address_collect, current_pid_info->length_collect, SMART_MADVISE_TASK_MADVISE, SMART_MADVISE_SEQUENTIAL);
                        current_pid_info->state = SMART_MADVISE_STATE_SEQUENTIAL;
                    }
                    break;
                default:
                    break;
            }
            
        }
        pid_data[idx].last_addr = page_addr;
    }
    // printk( "handle_pre_pagefault triggered\n");

    return 0;
}

void print_pid_data(void) {
    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        if  (pid_data[i].seq_access_count != 0) {
            printk( "PID %u: Last Addr: %llu, Access Count: %llu\n",
                   pid_data[i].pid,
                   (unsigned long )pid_data[i].last_addr,
                   (unsigned long )pid_data[i].seq_access_count);
        }
    }
}

void reset_pid_data(struct pid_info *pid_info, pid_t pid, u64 start, size_t length) {
    pid_info->pid = pid;
    pid_info->tracked = true;
    pid_info->start_address_collect = start;
    pid_info->length_collect = length;
    pid_info->seq_access_count = 0;
    pid_info->rand_access_count = 0;
    pid_info->state = SMART_MADVISE_STATE_INITIAL;
}
