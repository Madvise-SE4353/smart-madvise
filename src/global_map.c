#include <linux/slab.h>
#include <linux/mutex.h>
#include "global_map.h"

int init_task_map(global_task_map *task_map_ptr)
{
    // init the mutex
    mutex_init(&(task_map_ptr->mutex));
    // init the task map
    task_map_ptr->task_map = kcalloc(1, sizeof(task_item_node *), GFP_KERNEL);
    return 0;
}

int add_task(global_task_map *task_map_ptr, pid_t pid, unsigned long start, size_t len, task_class class, int flag)
{
    mutex_lock(&(task_map_ptr->mutex));
    // create new node
    task_item_node *new_node = kmalloc(sizeof(task_item_node), GFP_KERNEL);
    (new_node->task_item).pid = pid;
    (new_node->task_item).start = start;
    (new_node->task_item).len = len;
    (new_node->task_item).task_class = class;
    (new_node->task_item).task_flag = flag;
    // add the node to the list
    new_node->next = (task_map_ptr->task_map)[0];
    (task_map_ptr->task_map)[0] = new_node;
    mutex_unlock(&(task_map_ptr->mutex));
    return 0;
}

task_item_node *get_task_list(global_task_map *task_map_ptr, pid_t pid)
{
    task_item_node *result = NULL;
    mutex_lock(&(task_map_ptr->mutex));
    result = (task_map_ptr->task_map)[0];
    (task_map_ptr->task_map)[0] = NULL;
    mutex_unlock(&(task_map_ptr->mutex));
    return result;
}

void release_task_list(task_item_node *node_ptr)
{
    task_item_node *current_ptr = node_ptr;
    task_item_node *next_ptr;
    while (current_ptr != NULL)
    {
        next_ptr = current_ptr->next;
        kfree(current_ptr);
        current_ptr = next_ptr;
    }
}