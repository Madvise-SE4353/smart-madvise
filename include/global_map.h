#ifndef GLOBAL_MAP_H
#define GLOBAL_MAP_H
#include <asm/ptrace.h>
#include <linux/mutex.h>
typedef enum __task_class
{
    SMART_MADVISE_TASK_MADVISE,
    SMART_MADVISE_TASK_MLOCK
} task_class;

typedef struct __task_item
{
    pid_t pid;
    unsigned long start;
    size_t len;
    task_class task_class;
    int task_flag;
} task_item;

typedef struct __task_item_node
{
    task_item task_item;
    struct __task_item_node *next;
} task_item_node;

typedef struct __global_task_map
{
    struct mutex mutex;
    task_item_node **task_map;
} global_task_map;

extern global_task_map task_map_global;

int init_task_map(global_task_map *task_map_ptr);
int add_task(global_task_map *task_map_ptr, pid_t pid, unsigned long start, size_t len, task_class class, int flag);
task_item_node *get_task_list(global_task_map *task_map_ptr, pid_t pid);
void release_task_list(task_item_node *node_ptr);

#endif