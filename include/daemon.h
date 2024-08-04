#ifndef DAEMON_H
#define DAEMON_H

int smart_madvise_daemon(void *data);
int smart_madvise_start_daemon(void);
void smart_madvise_stop_daemon(void);

#endif