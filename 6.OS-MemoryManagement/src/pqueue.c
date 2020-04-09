#include <stdlib.h>
#include <pnode.h>
#include <pqueue.h>
#include <request.h>

//Initialize queue
pqueue_t* init_pqueue(){
	pqueue_t* q = (pqueue_t*)malloc(sizeof(pqueue_t));
	q->front = NULL;
	q->rear = NULL;
	q->size = 0;
	return q;
}

// Take out queue and return node
pnode_t* depqueue(pqueue_t* q){
	if (q->front == NULL){
		return NULL;
	}

	// Update front
	pnode_t* temp = q->front;
	q->front = q->front->next;
	q->size--;
	if (q->front == NULL){
		q->size = 0;
		q->rear = NULL;
	}
	return temp;
}

//Add item to queue
int enpqueue(pqueue_t* q, request_t request){
	pnode_t* temp = new_pnode(request);	

	if (q->rear == NULL){
		q->front = temp;
		q->rear = temp;
		q->size = 1;
		return q->size;
	}
	else{
		q->rear->next = temp;
		q->rear = temp;
		q->size++;
	}
	return q->size;
}
