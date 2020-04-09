#ifndef SHMRY_H
#define SHMRY_H
#include "shmsg.h"
#include <semaphore.h>
#include <resources.h>

#define NANOSEC 1000
#define SECOND 1000
#define PERMS 0600
#define SNAME "/mutex"

typedef struct shmry_t{
	int sec;
	int nanosec;
	struct shmsg_t msg;
	struct rdescriptor descriptor[20];
	struct request_t r_request;
	struct request_t r_granted;
	int max_claim[18][20];
	int r_allocation[18][20];
	sem_t* mutex;
} shmry_t;
#endif

