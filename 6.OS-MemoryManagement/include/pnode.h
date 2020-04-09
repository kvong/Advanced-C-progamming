#ifndef PNODE_H
#define PNODE_H
#include <request.h>

typedef struct pnode_t {
	request_t request;
	struct pnode_t* next;
}pnode_t;

pnode_t* new_pnode(request_t);
#endif /* end of include guard: PNODE_H */
