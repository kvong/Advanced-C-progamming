#ifndef RNODE_H
#define RNODE_H
#include <resources.h>

typedef struct node_r {
	request_t request;
	struct node_r* next;
}node_r;

node_r* new_rnode(request_t);
#endif /* end of include guard: NODE_H */
