#ifndef EXECUTOR_H
#define EXECUTOR_H
#include <asm/ptrace.h>
#include <linux/kprobes.h>

void schedule_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags);

#endif