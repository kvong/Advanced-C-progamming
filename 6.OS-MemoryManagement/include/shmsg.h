#ifndef SHMSG_H
#define SHMSG_H
typedef struct shmsg_t{
	int sec;
	int nanosec;
	int empty;
} shmsg_t;
#endif
