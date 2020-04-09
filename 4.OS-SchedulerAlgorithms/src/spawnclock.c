#include <shmry.h>

#define MAXSECBP 1
#define MAXNANBP SECOND

unsigned int get_randsec(){
	return rand() % MAXSECBP;		
}

unsigned int get_randnan(){
	return rand() % MAXNANBP;	
}
