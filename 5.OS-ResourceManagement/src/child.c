#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "shmry.h"
#include <rnode.h>
#include <sys/msg.h>
#include <msgqueue.h>

/******************************************************************************
* File:             child.c
*
* Author:           Khanh Vong  
* Created:          11/10/19 
*                   Child process for OSS that simulate random
*                   request/release/termination of applications.
*****************************************************************************/

// Prototypes
int releasable(int [20], int);
int get_action();
int requestable(int [20], int[20], int);
int main(int argc, char** argv){
	// Set up
	shmry_t* shm_ptr;
	int shm_id;
	int msg_rcv_id, msg_snd_id;
	msgqueue_t m_queue;
	key_t key;
	int i, j, looper = 1;
	int local_pid = atoi(argv[0]);
	int mtype = local_pid + 1;

	int start_sec, start_nan;	
	int option = 0, waiting = 0;
	int r_type = -1, amount = 0;

	int max_claim[20];
	srand(time(NULL)*getpid());

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

	// Set Max Claim
	for (i = 0; i < 20; i++) {
		if (shm_ptr[0].descriptor[i].resource != 0) {
			max_claim[i] = rand() % shm_ptr[0].descriptor[i].resource;
		}
		else 
			max_claim[i] = 0;
		shm_ptr[0].max_claim[local_pid][i] = max_claim[i];
	}

	start_sec = shm_ptr[0].sec;
	start_nan = shm_ptr[0].nanosec;

	while(1){
		if (start_sec < shm_ptr[0].sec){
			option = get_action(1);
		}
		else
			option = get_action(0);

		// Receiving message to enter the critical section
		if (msgrcv(msg_rcv_id, &m_queue, sizeof(m_queue.msg), mtype, 0) == -1){
			perror("dostuff: msgrcv");	
			return -1;
		}
		if (m_queue.msg.local_pid == -100) {
			return 0;
		}

		switch (option) {
			case 0:
				// Request resources
				if (requestable(shm_ptr[0].max_claim[local_pid], shm_ptr[0].r_allocation[local_pid], 20) == 1){
					while(looper == 1){
						r_type = rand()% 20;
						if (shm_ptr[0].max_claim[local_pid][r_type] > shm_ptr[0].r_allocation[local_pid][r_type]) {
							looper = 0;
						}
					}
					looper = 1;
					if ((shm_ptr[0].max_claim[local_pid][r_type] - shm_ptr[0].r_allocation[local_pid][r_type]) == 1) {
						amount = 1;
					}
					else{
						amount = rand()% (shm_ptr[0].max_claim[local_pid][r_type] - shm_ptr[0].r_allocation[local_pid][r_type] - 1) + 1;
					}
				}
				else{
					// If theres no more resources to request
					amount = 0;
					r_type = 0;
				}
				break;
			case 1:
				// Release resources
				if (releasable(shm_ptr[0].r_allocation[local_pid], 20) == 1) {
					while(looper == 1){
						r_type = rand()% 20;
						if (shm_ptr[0].r_allocation[local_pid][r_type] != 0) {
							looper = 0;
						}
					}
					looper = 1;
					amount = rand()% shm_ptr[0].r_allocation[local_pid][r_type];
					if (amount == 0)
						amount++;
				}
				else{
					// If theres no resources to release
					amount = 0;
					r_type = 0;
				}
				break;
			case 2:
				// Simply terminate by sending option = 2 to OSS
				break;
		}

		m_queue.mtype = local_pid + 1;
		m_queue.msg.local_pid = local_pid;
		m_queue.msg.option = option;
		m_queue.msg.r_amount = amount;
		m_queue.msg.r_type = r_type;
		m_queue.msg.start_sec = start_sec;
		m_queue.msg.start_nanosec = start_nan;

		// Send decision to OSS
		if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0) == -1){
			perror("Master: processor(): msgsnd()");
			return -1;
		}
		if (option == 2) {
			break;
		}
	}

	return local_pid;
}

int releasable(int r_block[], int size){
	int i;
	for (i = 0; i < size; i++) {
		if (r_block[i] > 0)
			return 1;
	}
	return -1;
}

int requestable(int max_claim[20], int allocated[20], int size){
	int i;
	for (i = 0; i < size; i++) {
		if (max_claim[i] > allocated[i]) {
			return 1;
		}
	}
	return -1;
}

int array_zero(int blockl[20]){
	int i = 0;
	for (i = 0; i < 20; i++) {
		if (blockl[i] != 0) {
			return -1;
		}
	}
	return 1;
}

int get_action(int t_flag){
	int option = rand() % 100 + 1;
	if (option < 10 && t_flag == 1) {
		return 2;
	}
	else if (option < 25) {
		return 1;
	}
	else
		return 0;
}
