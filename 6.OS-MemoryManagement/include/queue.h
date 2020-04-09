#ifndef QUEUE_H
#define QUEUE_H
#include <node.h>

typedef struct queue_t {
	struct node_t* front;
	struct node_t* rear;
	int size;
}queue_t;

queue_t* init_queue();
node_t* dequeue();
int enqueue(queue_t*, int, int);

#endif /* end of include guard: QUEUE_H */
