#ifndef _PAGECACHE_COLLECTOR_H
#define _PAGECACHE_COLLECTOR_H

#include <linux/types.h>
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


#define HASH_SIZE 256

#define PID_DATA_SIZE 256

struct pid_info {
    bool tracked;
    u64 last_addr;
    u64 access_count;
    u64 start_address_collect;
    size_t length_collect;
    u32 pid;
};


int hash_pid(u32 pid);
int handle_pre_pagefault(struct kprobe *p, struct pt_regs *regs);
void print_pid_data(void);

#endif
