#include <stdlib.h>
#include <prblock.h>
#include <pnode.h>

node_p* new_pnode(prblock_t block){
	node_p* temp = (node_p*)malloc(sizeof(node_p));
	temp->block = block;
	temp->next = NULL;
	return temp;
}
