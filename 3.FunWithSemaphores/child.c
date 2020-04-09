#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "shmry.h"

// Author: Khanh Vong
// Date: 10/4/2019
// Description: Child process to be executed by OSS which will get a random termination time.
//				Multiple processes will be launched so a semaphore lock on a critical section
//				must be enforced.

int main(int argc, char** argv){
	shmry_t* shm_ptr;
	sem_t* sem;
	int shm_id;
	key_t key;

	int sec, nanosec;
	int rand_num, term_sec, term_nanosec;
	srand(time(NULL));

	// Get semaphore created by parent
	if ((sem = sem_open(SNAME, 0)) == SEM_FAILED){
		//perror("dostuff: Error: sem_open");	
		return 1;
	}

	// Shared memory
	key = ftok("./", 100);
	// Get memory
	shm_id = shmget(key, sizeof(shmry_t), IPC_CREAT | PERMS);
	if (shm_id == -1){
		perror("dostuff: Error: shm_id");
		return 1;
	}
	// Attach memory to shm_ptr
	shm_ptr = (int *)shmat(shm_id, NULL, 0);
	if ( shm_ptr == -1 ){
		perror("dostuff: Error: shmat");
		return 1;
	}

	//
	sec = shm_ptr[0].sec;
	nanosec = shm_ptr[0].nanosec;
	rand_num = rand() % 1000000 + 1;
	term_nanosec = rand_num + nanosec;
	term_sec = sec;
	// SECOND = how many nanoseconds in second
	if (term_nanosec >= SECOND){
		term_nanosec = term_nanosec % SECOND;	
		term_sec = sec + 1;
	}
	// Loop through critical section
	while(1){
		// Loop over semaphore until we can get in
		while(sem_wait(sem) == -1){
			if(errno != EINTR){
					perror("dostuff: Error: sem_wait");
					shmdt(shm_ptr);
					return 1;
			}
		}
	//------------------------------------------------> CS ENTRY
	//	If term_time is up and shmsg is empty
		if (term_nanosec <= shm_ptr[0].nanosec && term_sec <= shm_ptr[0].sec && shm_ptr[0].msg.empty){
			shm_ptr[0].msg.nanosec = term_nanosec;
			shm_ptr[0].msg.sec = term_sec;
			shm_ptr[0].msg.empty = 0;
			break; // breaking out of the while(1)
		}
		// If not our time yet give use of CS to another process
		else{
			if(sem_post(sem) == -1){
				perror("dostuff: Error: sem_wait");	
				exit(1);
			}
		}
	}
	//------------------------------------------------> CS EXIT
	if(sem_post(sem) == -1){
		perror("dostuff: Error: sem_wait");	
		return 1;
	}
	// Return the child process number (not pid) to parent 
	// so the parent know which of its child has terminated.
	return atoi(argv[0]);
}
