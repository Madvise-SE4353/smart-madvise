#include <linux/kprobes.h>
#include <asm/ptrace.h>
#include "myprintk.h"



void myprintk(){
    printk("do_madivse myprintk\n");
}