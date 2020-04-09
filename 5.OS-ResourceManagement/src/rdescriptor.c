#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <rdescriptor.h>

rdescriptor* init_descriptor(int lbound, int ubound, int size){
	int i;
	srand(time(NULL));
	
	rdescriptor* descriptor = (rdescriptor*)malloc(sizeof(rdescriptor)*size);
	for (i = 0; i < size; i++) {
		descriptor[i].resource = rand() % ubound + lbound;
	}
	return descriptor;
}

void print_descriptor(rdescriptor* descriptor, int size){
	int i;
	for (i = 0; i < size; i++) {
		printf(" R%d: %d |", i, descriptor[i].resource);
	}
	printf("\n");
}
