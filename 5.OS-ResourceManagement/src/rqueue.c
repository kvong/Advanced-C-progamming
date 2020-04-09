#include <stdlib.h>
#include <rnode.h>
#include <rqueue.h>

//Initialize queue
queue_r* init_rqueue(){
	queue_r* q = (queue_r*)malloc(sizeof(queue_r));
	q->front = NULL;
	q->rear = NULL;
	q->size = 0;
	return q;
}

// Take out queue and return node
node_r* derqueue(queue_r* q){
	if (q->front == NULL){
		return NULL;
	}

	// Update front
	node_r* temp = q->front;
	q->front = q->front->next;
	q->size--;
	if (q->front == NULL){
		q->size = 0;
		q->rear = NULL;
	}
	return temp;
}

//Add item to queue
int enrqueue(queue_r* q, request_t request){
	node_r* temp = new_rnode(request);	

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
