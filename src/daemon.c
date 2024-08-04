#include <linux/kthread.h>
#include <linux/delay.h>

#include "daemon.h"
#include "pagecache_collector.h"

struct task_struct *daemon_task_struct = NULL;

int smart_madvise_daemon(void *data)
{
	while (!kthread_should_stop()) {
		struct list_head *pos;
		struct pid_info *pid_info;
		list_for_each(pos, &pid_data_list[0]) {
			pid_info = list_entry(pos, struct pid_info, list_node);
			pr_info("smart-madvise: pid %d, access count %llu\n", pid_info->pid, pid_info->access_count);
		}
		// pr_info("smart-madvise: daemon running\n");
		msleep(1000);
	}
	return 0;
}

int smart_madvise_start_daemon(void)
{
	struct task_struct *daemon;
	daemon = kthread_run(smart_madvise_daemon, NULL, "smart-madvise-daemon");
	if (IS_ERR(daemon)) {
		pr_err("smart-madvise: failed to create daemon\n");
		return PTR_ERR(daemon);
	}
	daemon_task_struct = daemon;
	return 0;
}

void smart_madvise_stop_daemon(void)
{
	if (daemon_task_struct) {
		kthread_stop(daemon_task_struct);
	}
}
