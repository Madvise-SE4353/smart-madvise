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

enum smart_madvise_state {
    SMART_MADVISE_STATE_INITIAL,
    SMART_MADVISE_STATE_SEQUENTIAL,
    SMART_MADVISE_STATE_RANDOM,
    // maybe more
};

struct pid_info {
    bool tracked;
    u64 last_addr;
    u64 access_count;
    u64 start_address_collect;
    size_t length_collect;
    u32 pid;
    enum smart_madvise_state state;
    struct list_head list_node;
};

extern struct list_head pid_data_list[1]; // maybe add multiple priotity levels

void reset_pid_data(struct pid_info *pid_info, pid_t pid, u64 start, size_t length);
int hash_pid(u32 pid);
int handle_pre_pagefault(struct kprobe *p, struct pt_regs *regs);
void print_pid_data(void);

#endif
