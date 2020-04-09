#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shmry.h>
#include <time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <msgqueue.h>

/******************************************************************************
* File:             child.c
*
* Author:           Khanh Vong  
* Created:          10/23/19 
* Description:      Simulated process for simulated OS. Simulate the different action taken by a process such as interupts, block, full run, and termination.
*****************************************************************************/

int get_option();
int main(int argc, char *argv[])
{
	int local_pid = atoi(argv[0]);
	int mtype = local_pid + 1;
	shmry_t* shm_ptr;
	int shm_id;
	int msg_rcv_id, msg_snd_id;
	msgqueue_t m_queue;
	
	key_t key;

	int looper = 1;
	int i, s, n, p;
	int sec, nanosec;
	int p_option;

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

	// Option 0 - Terminate program;
	// Option 1 - Run for whole time quantum;
	// Option 2 - Wait for a random event that will last s sec and n nanosec;
	// Option 3 - Use p time quantum and wait;
	
	// Ensure that we arent using the same seed
	srand(time(NULL) * getpid());

	// Keep looping until termination call is rolled.
	while(looper == 1){
		p_option = get_option();

		// Receiving message
		if (msgrcv(msg_rcv_id, &m_queue, sizeof(m_queue.pc_block), mtype, 0) == -1){
			printf("%d:\n", local_pid);
			perror("dostuff: msgrcv:");	
			return -1;
		}
		switch (p_option) {
			case 0:
				// Terminate
				shm_ptr[0].p_block[local_pid].child_option = 0;
				shm_ptr[0].p_block[local_pid].time_used_sec = 0;
				shm_ptr[0].p_block[local_pid].time_used_nanosec = 0;
				shm_ptr[0].p_block[local_pid].wait_sec = 0;
				shm_ptr[0].p_block[local_pid].wait_nanosec = 0;
				shm_ptr[0].p_block[local_pid].active = 0;
				looper = -1;
				break;
			case 1:
				// Use whole time quantum
				shm_ptr[0].p_block[local_pid].child_option = 1;
				shm_ptr[0].p_block[local_pid].time_used_sec = 0;
				shm_ptr[0].p_block[local_pid].time_used_nanosec = m_queue.pc_block.t_quantum;
				shm_ptr[0].p_block[local_pid].wait_sec = 0;
				shm_ptr[0].p_block[local_pid].wait_nanosec = 0;
				shm_ptr[0].p_block[local_pid].active = 1;
				break;
			case 2:
				// Wait for event that last for s sec and n nanosec;
				s = rand() % 6;
				n = rand() % 1001;
				shm_ptr[0].p_block[local_pid].child_option = 2;
				shm_ptr[0].p_block[local_pid].time_used_sec = 0;
				shm_ptr[0].p_block[local_pid].time_used_nanosec = 0;
				shm_ptr[0].p_block[local_pid].wait_nanosec = shm_ptr[0].nanosec + n;
				shm_ptr[0].p_block[local_pid].wait_sec = shm_ptr[0].sec + s;
				if (shm_ptr[0].p_block[local_pid].wait_nanosec/SECOND == 1){
					shm_ptr[0].p_block[local_pid].wait_sec++;
					shm_ptr[0].p_block[local_pid].wait_nanosec = shm_ptr[0].p_block[local_pid].wait_nanosec % SECOND;
				}
				shm_ptr[0].p_block[local_pid].active = 1;
				break;
			case 3:
				// Use p time quantum and then interrupted 
				p = rand() % 99 + 1;
				shm_ptr[0].p_block[local_pid].child_option = 3;
				shm_ptr[0].p_block[local_pid].time_used_sec = 0;
				shm_ptr[0].p_block[local_pid].time_used_nanosec = p;
				shm_ptr[0].p_block[local_pid].wait_sec = 0;
				shm_ptr[0].p_block[local_pid].wait_nanosec = 0;
				shm_ptr[0].p_block[local_pid].active = 1;
				break;
		}

		// Set up message to send to parent
		m_queue.mtype = local_pid + 1;
		m_queue.pc_block = shm_ptr[0].p_block[local_pid];
		// Sending back to message
		if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.pc_block), 0) == -1){
			perror("Master: processor(): msgsnd()");
			return -1;
		}
	}
	return local_pid;
}

int get_option(){
	int p_execute;
	// Pick execution condition
	p_execute = rand() % 100 + 1;
	if (p_execute < 10){
		p_execute = 0;
	}
	else if (p_execute >= 10 && p_execute < 40) {
		p_execute = 1;
	}
	else if (p_execute >= 40 && p_execute < 70) {
		p_execute = 2;
	}
	else{
		p_execute = 3;
	}
	return p_execute;
}
