#include <linux/kprobes.h>
#include <asm/ptrace.h>

MODULE_AUTHOR("Haocheng Wang");
MODULE_DESCRIPTION("Customized Madvise");
MODULE_LICENSE("GPL");

static int sys_madvise_kprobe_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct pt_regs *madvise_regs = (struct pt_regs *)(regs->di);
    printk("call customized madvise, di 0x%lX, si 0x%lX, dx %lu\n", madvise_regs->di, madvise_regs->si, madvise_regs->dx);
    madvise_regs->dx = 0;
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

    return 0;
}

static void __exit my_module_exit(void)
{
    printk("my_module_exit\n");
    unregister_kprobe(&syscall_kprobe);
}

module_init(my_module_init);
module_exit(my_module_exit);