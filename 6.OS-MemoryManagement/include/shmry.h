#ifndef SHMRY_H
#define SHMRY_H
#include "shmsg.h"
#include <semaphore.h>

#define NANOSEC 1000
#define SECOND 10000
#define PERMS 0600
#define SNAME "/mutex"

typedef struct shmry_t{
	int sec;
	int nanosec;
	sem_t* mutex;
} shmry_t;
#endif

