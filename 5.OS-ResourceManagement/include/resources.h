#ifndef RDESCRIPTOR_H
#define RDESCRIPTOR_H

typedef struct rdescriptor{
	int resource;	
	int shareable;	
} rdescriptor;

typedef struct request_t {
	int local_pid;
	int option;
	int r_type;
	int r_amount;
	int start_sec;
	int start_nanosec;
	int wait_sec;
	int wait_nanosec;
}request_t;

#endif /* end of include guard: RDESCRIPTOR_H */
