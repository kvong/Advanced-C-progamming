#include <stdio.h>	
#include <shmry.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <msgqueue.h>
#include <semaphore.h>
#include <errno.h>
#include <request.h>

#define MINREF 100

/******************************************************************************
* File:             child.c
*
* Author:           Khanh Vong  
* Created:          12/03/19 
*                   Process simulator to work with OS simulator to simulate
*                   random write/read request for its page.
*****************************************************************************/


int main(int argc, char *argv[])
{
	// Set up
	shmry_t* shm_ptr;
	sem_t* sem;
	int shm_id;
	int msg_rcv_id, msg_snd_id;
	msgqueue_t m_queue;
	key_t key;
	int local_pid = atoi(argv[0]);
	int max_mem = atoi(argv[1]);

	int mtype = local_pid + 1;
	int start_sec, start_nan;	
	int mem_request;
	int mem_occupied = 0;
	int mem_referenced = 0;
	int operation;
	int referenced_total;
	int upperbound = local_pid*32*1024 + max_mem*1024;
	int lowerbound = local_pid*32*1024;

	srand(time(NULL)*getpid());

	// Get semaphore created by parent
	if ((sem = sem_open(SNAME, 0)) == SEM_FAILED){
		//perror("dostuff: Error: sem_open");	
		return 1;
	}

	// Shared memory
	key = 6666;
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
	
	// Message queue for recieving
	key = 7777;
	if((msg_rcv_id = msgget(key, IPC_CREAT | PERMS)) < 0){
		perror("dostuff: msgget:");
		return -1;
	}

	// Message queue for recieving
	key = 8888;
	if((msg_snd_id = msgget(key, IPC_CREAT | PERMS)) < 0){
		perror("dostuff: msgget:");
		return -1;
	}

	referenced_total = rand() % 200 + MINREF;

	// Loop through critical section
	while(1){
		mem_request = rand() % (upperbound - lowerbound);
		operation = rand() % 2; // 0 = read; 1 = write;

		// Loop over semaphore until we can get in
		if(sem_wait(sem) == -1){
			if(errno != EINTR){
					perror("dostuff: Error: sem_wait");
					shmdt(shm_ptr);
					return 1;
			}
		}
		//------------------------------------------------> CS ENTRY
		m_queue.mtype = mtype;
		m_queue.msg.local_pid = local_pid;

		m_queue.msg.operation = operation;
		m_queue.msg.request_addr = lowerbound + mem_request;
		m_queue.msg.page_number = mem_request/1024;
		m_queue.msg.request_sec = shm_ptr[0].sec;
		m_queue.msg.request_nanosec = shm_ptr[0].nanosec;

		// Send request to OS
		if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0) == -1){
			perror("Master: processor(): msgsnd()");
			return -1;
		}

		// Give up turn
		if(sem_post(sem) == -1){
			perror("dostuff: Error: sem_wait");	
			return 1;
		}
		
		// Wait for request to be granted
		if (msgrcv(msg_rcv_id, &m_queue, sizeof(m_queue.msg), mtype, 0) == -1){
			perror("dostuff: msgrcv");	
			return -1;
		}

		mem_referenced++;
		//------------------------------------------------> CS EXIT
		if (mem_referenced > referenced_total){
			m_queue.msg.request_addr = -1;
			m_queue.msg.page_number = -1;
			if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0) == -1){
				perror("Master: processor(): msgsnd()");
				return -1;
			}
			break;
			//------------------------------------------------> CS EXIT
		}
	}
	return local_pid;
}
