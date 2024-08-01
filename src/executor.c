#include <asm/ptrace.h>
#include "executor.h"
#include "global_map.h"
#include "pagecache_collector.h"

typedef int (*syscall_wrapper)(struct pt_regs *);

// extern pid_t register_pid;
extern struct pid_info pid_data[HASH_SIZE];
extern global_task_map task_map_global;
extern syscall_wrapper original_madvise;

static void execute_task_list(task_item_node *node_ptr);
static void execute_madvise(task_item *task_ptr);
static void execute_mlock(task_item *task_ptr);

void schedule_post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    pid_t pid = current->pid; 
    int idx = hash_pid(pid);
    struct pid_info *current_pid_info = &pid_data[idx];

    if (current_pid_info->tracked && current_pid_info->pid == pid) {
        // printk("schedule to register_pid %d\n", register_pid);
        task_item_node *list = get_task_list(&task_map_global, pid);
        execute_task_list(list);
        release_task_list(list);
    }
}

static void execute_task_list(task_item_node *node_ptr)
{
    task_item_node *current_ptr = node_ptr;
    while (current_ptr != NULL)
    {
        switch ((current_ptr->task_item).task_class)
        {
        case SMART_MADVISE_TASK_MADVISE:
        {
            execute_madvise(&(current_ptr->task_item));
            break;
        }
        case SMART_MADVISE_TASK_MLOCK:
        {
            execute_mlock(&(current_ptr->task_item));
            break;
        }
        default:
        {
            // WARNING: shouldn't reach here
            break;
        }
        }
        current_ptr = current_ptr->next;
    }
}

static void execute_madvise(task_item *task_ptr)
{
    struct pt_regs regs;
    regs.di = task_ptr->start;
    regs.si = task_ptr->len;
    regs.dx = task_ptr->task_flag;
    int result = original_madvise(&regs);
    printk("execute_madvise, di 0x%lX, si %ld, dx %d, result %d\n", regs.di, regs.si, regs.dx, result);
}

static void execute_mlock(task_item *task_ptr)
{
    // TODO: finish the function
}