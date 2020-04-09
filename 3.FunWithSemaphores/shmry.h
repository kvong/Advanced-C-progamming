#ifndef SHMRY_H
#define SHMRY_H
#include "shmsg.h"
#include <semaphore.h>

#define NANOSEC 100000
#define SECOND 1000000000
#define PERMS 0600
#define SNAME "/mutex"

typedef struct shmry_t{
	int sec;
	int nanosec;
	struct shmsg_t msg;
	sem_t* mutex;
} shmry_t;
#endif

