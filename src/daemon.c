#include "daemon.h"
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

static struct task_struct *kth_arr[4];
static struct smart_madvise_daemon_ctx ctx[4];

static int thread_function(void *ctx)
{
	unsigned int i = 0;
	struct smart_madvise_daemon_ctx *ctx_p =
		(struct smart_madvise_daemon_ctx *)ctx;
	while (!kthread_should_stop()) {
		pr_info("Thread %d Still running...! %d secs\n", ctx_p->n, i);
		i++;
		if (i == 30)
			break;
		msleep(1000);
	}
	pr_info("thread %d stopped\n", ctx_p->n);
	return 0;
}

static int initialize_thread(struct task_struct **kth,
			     struct smart_madvise_daemon_ctx *ctx)
{
	char th_name[20];
	sprintf(th_name, "kthread_%d", ctx->n);
	*kth = kthread_run(thread_function, ctx, (const char *)th_name);
	if (*kth == NULL) {
		pr_err("kthread %s could not be created\n", th_name);
		return -1;
	}
	return 0;
}

int smart_madvise_start_daemon()
{
	int i = 0;
	pr_info("Initializing smart madvise daemon");
	for (i = 0; i < 4; i++) {
		// initialize one thread at a time.
		ctx[i].n = i;
		if (initialize_thread(&kth_arr[i], &ctx[i]) == -1) {
			return -1;
		}
	}
	printk(KERN_INFO "all of the threads are running\n");
	return 0;
}

void smart_madvise_stop_daemon()
{
	int i = 0;
	int ret = 0;
	pr_info("stopping daemon\n");
	for (i = 0; i < 4; i++) {
		// stop all of the threads before removing the module.
		ret = kthread_stop(kth_arr[i]);
		if (ret) {
			pr_err("can't stop thread %d, retval = %d\n", i, ret);
		}
	}
	printk(KERN_INFO "stopped all of the threads\n");
}
