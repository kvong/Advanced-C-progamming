#ifndef PQUEUE_H
#define PQUEUE_H
#include <pnode.h>
#include <request.h>

typedef struct pqueue_t {
	struct pnode_t* front;
	struct pnode_t* rear;
	int size;
}pqueue_t;

pqueue_t* init_pqueue();
pnode_t* depqueue(pqueue_t*);
int enpqueue(pqueue_t*, request_t);
#endif /* end of include guard: PQUEUE_H */
