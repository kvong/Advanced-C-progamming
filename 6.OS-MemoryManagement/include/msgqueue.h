#ifndef MSGQUEUE_H
#define MSGQUEUE_H
#include <request.h>

typedef struct msgqueue_t {
	long mtype;
	request_t msg;
}msgqueue_t;

#endif /* end of include guard: MSGQUEUE_H */
