#ifndef PQUEUE_H
#define PQUEUE_H
#include <pnode.h>
#include <prblock.h>

typedef struct queue_p {
	int wait_time;
	int size;
	struct node_p* front;
	struct node_p* rear;
}queue_p;

queue_p* init_pqueue();
node_p* depqueue(queue_p*);
int enpqueue(queue_p*, prblock_t);
#endif /* end of include guard: PQUEUE_H */
