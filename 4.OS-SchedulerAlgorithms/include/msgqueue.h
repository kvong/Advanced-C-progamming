#ifndef MSGQUEUE_H
#define MSGQUEUE_H
#include <prblock.h>

typedef struct msgqueue_t {
	long mtype;
	prblock_t pc_block;
}msgqueue_t;

#endif /* end of include guard: MSGQUEUE_H */
