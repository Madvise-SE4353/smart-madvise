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

struct pid_info {
    u64 last_addr;
    u64 access_count;
    u32 pid;
};

static struct pid_info pid_data[HASH_SIZE];
static struct kprobe kp;
static struct proc_dir_entry *proc_file;

static int hash_pid(u32 pid) {
    return hash_long(pid, 8) % HASH_SIZE;
}

// Kprobe pre-handler: called just before the probed instruction is executed
static int handle_pre_fault(struct kprobe *p, struct pt_regs *regs) {
    struct vm_area_struct *vma = (struct vm_area_struct *)regs->di;
    unsigned long address = regs->si;
    unsigned int flags = regs->dx;
    u32 pid = current->pid;
    int idx = hash_pid(pid);
    u64 page_addr = address >> 12;
    // filter here 
    if (pid_data[idx].last_addr + 1 == page_addr) {
        pid_data[idx].access_count++;
    }
    // else
    //     pid_data[idx].access_count
    pid_data[idx].last_addr = page_addr;
    pid_data[idx].pid = pid;

    return 0;
}

// Read function for the proc file
static int page_faults_proc_show(struct seq_file *m, void *v) {
    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        if (pid_data[i].access_count != 0) {
            seq_printf(m, "PID %u: Sequential Access Count: %llu\n", pid_data[i].pid, pid_data[i].access_count);
        }
    }
    return 0;
}

static int page_faults_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, page_faults_proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open = page_faults_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init kprobe_init(void) {
    printk(KERN_INFO "Initializing the kprobe module for sequential page faults.\n");
    memset(pid_data, 0, sizeof(pid_data));

    kp.pre_handler = handle_pre_fault;
    kp.symbol_name = "handle_mm_fault";

    if (register_kprobe(&kp) < 0) {
        printk(KERN_INFO "Failed to register kprobe\n");
        return -1;
    }

    printk(KERN_INFO "Kprobe registered at %p\n", kp.addr);

    // Create proc file
    proc_file = proc_create("seq_page_faults", 0, NULL, &proc_fops);
    if (!proc_file) {
        printk(KERN_ALERT "Error: Could not initialize /proc/seq_page_faults\n");
        return -ENOMEM;
    }

    return 0;
}

static void __exit kprobe_exit(void) {
    unregister_kprobe(&kp);
    proc_remove(proc_file);
    printk(KERN_INFO "Kprobe module and proc file unloaded.\n");
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vahagn Ghazaryan");
MODULE_DESCRIPTION("Kernel module using kprobes to track sequential page faults and expose data through procfs.");
// TODO consider address, filter pid, start and length of the page
