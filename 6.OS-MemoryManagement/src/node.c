#include <stdlib.h>
#include <node.h>

node_t* new_node(int local_pid, int frame_number){
	node_t* tmp = (node_t*)malloc(sizeof(node_t));
	tmp->local_pid = local_pid;
	tmp->frame_number = frame_number;
	tmp->next = NULL;
	return tmp;
}
