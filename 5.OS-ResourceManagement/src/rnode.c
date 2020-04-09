#include <stdlib.h>
#include <rnode.h>
#include <resources.h>

node_r* new_rnode(request_t request){
	node_r* temp = (node_r*)malloc(sizeof(node_r));
	temp->request = request;
	temp->next = NULL;
	return temp;
}
