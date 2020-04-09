#ifndef RDESCRIPTOR_H
#define RDESCRIPTOR_H

typedef struct rdescriptor{
	int resource;
}rdescriptor;

rdescriptor* init_descriptor(int, int, int);
void print_descriptor(rdescriptor*, int);
#endif /* end of include guard: RDESCRIPTOR_H */
