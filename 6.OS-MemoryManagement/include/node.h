#ifndef NODE_H
#define NODE_H

typedef struct node_t {
	int local_pid;
	int frame_number;
	struct node_t* next;
}node_t;

node_t* new_node(int, int);
#endif /* end of include guard: NODE_H */
