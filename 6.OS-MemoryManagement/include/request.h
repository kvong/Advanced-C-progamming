#ifndef REQUEST_H
#define REQUEST_H

typedef struct request_t {
	int local_pid;
	int page_number;
	int request_addr;
	int operation;
	int grant_sec;
	int grant_nanosec;
	int request_sec;
	int request_nanosec;
}request_t;

#endif /* end of include guard: REQUEST_H */
