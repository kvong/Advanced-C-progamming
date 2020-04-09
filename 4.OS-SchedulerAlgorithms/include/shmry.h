#ifndef SHMRY_H
#define SHMRY_H
#include <prblock.h>

#define PCBSIZE 18
#define NANOSEC 13
#define SECOND 1000
#define PERMS 0600

typedef struct shmry_t{
	int nanosec;
	int sec;
	int clock_running;
	prblock_t p_block[PCBSIZE];
} shmry_t;

#endif
