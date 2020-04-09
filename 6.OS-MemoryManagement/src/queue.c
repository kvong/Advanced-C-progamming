#include <stdlib.h>
#include <node.h>
#include <queue.h>

// Initialize queue
queue_t* init_queue(){
	queue_t* q = (queue_t*)malloc(sizeof(queue_t));
	q->front = NULL;
	q->rear = NULL;
	q->size = 0;
	return q;
}

// Take out front of queue and return the node
node_t* dequeue(queue_t* q){
	if (q->front == NULL) {
		return NULL;
	}

	// Update front
	node_t* temp = q->front;
	q->front = q->front->next;
	q->size--;
	if (q->front == NULL) {
		q->size = 0;
		q->rear = NULL;
	}
	return temp;
}

int enqueue(queue_t* q, int local_pid, int frame_number){
	node_t* temp = new_node(local_pid,  frame_number);

	if (q->rear == NULL) {
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
