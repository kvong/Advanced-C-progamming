#ifndef PRBLOCK_H
#define PRBLOCK_H

typedef struct prblock_t {
	int active;
	int local_pid;
	int t_quantum;
	int child_option;
	int priority;
	unsigned int time_used_sec;
	unsigned int time_used_nanosec;
	unsigned int wait_sec;
	unsigned int wait_nanosec;
	unsigned int idle_sec;
	unsigned int idle_nano;
}prblock_t;

#endif /* end of include guard: PRBLOCK_H */
