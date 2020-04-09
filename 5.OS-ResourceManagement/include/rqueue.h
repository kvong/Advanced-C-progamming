#ifndef RQUEUE_H
#define RQUEUE_H
#include <rnode.h>
#include <resources.h>

typedef struct queue_r {
	int size;
	struct node_r* front;
	struct node_r* rear;
}queue_r;

queue_r* init_rqueue();
node_r* derqueue(queue_r*);
int enrqueue(queue_r*, request_t);
#endif /* end of include guard: PQUEUE_H */
