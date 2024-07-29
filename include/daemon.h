#ifndef _SMART_MADVISE_DAEMON_H
#define _SMART_MADVISE_DAEMON_H

struct smart_madvise_daemon_ctx {
	int n;
	// TODO: add more things in the context
};

int smart_madvise_start_daemon(void);
void smart_madvise_stop_daemon(void);

#endif