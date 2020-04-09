#ifndef PNODE_H
#define PNODE_H
#include <prblock.h>

typedef struct node_p {
	prblock_t block;
	struct node_p* next;
}node_p;

node_p* new_pnode(prblock_t);
#endif /* end of include guard: NODE_H */
