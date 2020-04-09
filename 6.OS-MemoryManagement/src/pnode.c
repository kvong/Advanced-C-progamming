#include <stdlib.h>
#include <pnode.h>
#include <request.h>

pnode_t* new_pnode(request_t request){
	pnode_t* temp = (pnode_t*)malloc(sizeof(pnode_t));
	temp->request.grant_sec = request.grant_sec;
	temp->request.grant_nanosec = request.grant_nanosec;
	temp->request.local_pid = request.local_pid;
	temp->request.page_number = request.page_number;
	temp->request.request_addr = request.request_addr;
	temp->request.operation = request.operation;
	temp->request.request_sec = request.request_sec;
	temp->request.request_nanosec = request.request_nanosec;
	temp->next = NULL;
	return temp;
}
