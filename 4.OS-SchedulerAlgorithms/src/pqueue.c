#include <stdlib.h>
#include <pnode.h>
#include <prblock.h>
#include <pqueue.h>

//Initialize queue
queue_p* init_pqueue(){
	queue_p* q = (queue_p*)malloc(sizeof(queue_p));
	q->front = NULL;
	q->rear = NULL;
	q->size = 0;
	return q;
}

// Take out queue and return node
node_p* depqueue(queue_p* q){
	if (q->front == NULL){
		return NULL;
	}

	// Update front
	node_p* temp = q->front;
	q->front = q->front->next;
	q->size--;
	if (q->front == NULL){
		q->size = 0;
		q->rear = NULL;
	}
	return temp;
}

//Add item to queue
int enpqueue(queue_p* q, prblock_t block){
	node_p* temp = new_pnode(block);	

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
