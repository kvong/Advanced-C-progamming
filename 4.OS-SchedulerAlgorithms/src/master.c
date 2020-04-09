#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>
#include <shmry.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <spawnclock.h>
#include <msgqueue.h>
#include <pqueue.h>

/******************************************************************************
* File:             master.c
*
* Author:           Khanh Vong  
* Created:          10/23/19 
*                   Operating system simulator with scheduling algorithm.
*****************************************************************************/

#define PRIOR 3
#define ALPHA 0.25
#define BETA 0.25
#define T_QUANTUM 1000

// Loose ends to take care of at termination
FILE* file_ptr = NULL;
pid_t child_pid[PCBSIZE];
static shmry_t* shm_ptr;
static msgqueue_t m_queue;
int shm_id, msg_snd_id, msg_rcv_id;
static queue_p* p_queue[PRIOR];
static queue_p* b_queue[PRIOR];

// Constants 
const int TIMER = 3;
// Process bound
const int PBOUND = 100;
// Dispatch time bound
const int D_UBOUND = 30;
const int D_LBOUND = 20;

// Wait threshold
const int S_WBOUND = 0;
const int NS_WBOUND = 1;

// Queue constants
const int QBOUND = 2;
const int PRIOR_1 = 0;
const int PRIOR_2 = 1;
const int PRIOR_3 = 2;

int p_tracker[PCBSIZE][9];

// Prototypes
int processor(char* output);
static void sig_handler(int);
void clean_file(char* input);
void cleanup();
int init_shmry(key_t);
int init_mqueue(key_t);
int has_running(int [PCBSIZE][9], int);
int has_opening(int [PCBSIZE][9], int);
int new_turn(int [PCBSIZE][9], int , int);
int main(int argc, char *argv[])
{
	srand(time(NULL));
	//Set up master alarm
	signal(SIGALRM, (void (*)(int))sig_handler);
	signal(SIGINT, (void (*)(int))sig_handler);
	alarm(TIMER);

	char output[20] = "Master.log";
	processor(output);
	
	perror("Program terminating");
	return 0;
}

void clean_file(char* input){
	file_ptr = fopen(input, "w");
	if(!file_ptr){
		printf("File '%d' could not be opened for cleaning.\n", input);
		perror("Master: Error: 'clean_file()' failed at fopen()");
		exit(EXIT_FAILURE);
	}
	fprintf(file_ptr, "");
	fclose(file_ptr);
}

int init_shmry(key_t key){
	shm_id = shmget(key, sizeof(shmry_t), IPC_CREAT | PERMS);
	if (shm_id == -1){
		perror("Master: Error: shm_id");
		return -1;
	}
	shm_ptr = (shmry_t *)shmat(shm_id, NULL, 0);
	if ( shm_ptr == -1 ){
		perror("Master: Error: shmat");
		return -1;
	}
	return 1;
}

int init_mqueue(key_t key){
	int msgid;
	if((msgid = msgget(key, IPC_CREAT | PERMS)) < 0){
		perror("Master: processor(): msgget:");
		return -1;
	}
	return msgid;
}

int has_running(int block[PCBSIZE][9], int size){
	int i;
	for (i = 0; i < size; i++) {
		if (block[i][0] != -1) {
			return i;
		}	
	}
	return -1;
}

int has_opening(int block[PCBSIZE][9], int size){
	int i;
	for (i = 0; i < size; i++) {
		if (block[i][0] == -1) {
			return i;
		}	
	}
	return -1;
}

int total_running(int block[PCBSIZE][9], int size){
	int i, total = 0;
	for (i = 0; i < size; i++) {
		if (block[i][0] != -1) {
			total++;
		}	
	}
	return total;
}

// Get next turn
int new_turn(int block[PCBSIZE][9], int c_turn, int turn_bound){
	int i;
	for (i = c_turn + 1; i < turn_bound; i++){
		if (block[i][0] != -1) {
			return i;
		}	
	}
	for (i = 0; i < c_turn; i++){
		if (block[i][0] != -1){
			return i;
		}
	}
	return c_turn;
}

// Control child processes
int processor(char* output){
	pid_t pid;
	node_p* p_node = NULL;

	int total_processes = 0;
	int index, stat = -1, i, j, m_type, local_pid, current_priority;
	int dispatch_time, child_option, used_sec, used_nanosec, wait_sec, wait_nanosec, active;

	int finish = -1;
	int finish_wait = -1;

	int wait_sec_sum;
	int wait_nanosec_sum;
	int avg_wait_sec;
	int avg_wait_nanosec;
	int waited_sec = 0;
	int waited_nanosec = 0;
	int waited_sec_sum = 0;
	int waited_nanosec_sum = 0;

	// Dequeue counters
	int q_counter = 0;
	int q1_counter = 0;
	int q2_counter = 0;
	int q3_counter = 0;

	// Priority based time quantum
	const int QUANTUM[3] = {T_QUANTUM, T_QUANTUM/2, T_QUANTUM/4};
	const double CONSTANT[2] = {ALPHA, BETA};
	printf("ALPHA = %d\n", CONSTANT[0]);
	printf("BETA = %d\n", CONSTANT[1]);

	// Initialize priority queues
	for (i = 0; i < PRIOR; i++) { 
		p_queue[i] = init_pqueue();
	}

	// Initialize priority block queues
	for (i = 0; i < PRIOR; i++) { 
		b_queue[i] = init_pqueue();
	}

	// Clean file for writing
	clean_file(output);

	// Open file for appending
	file_ptr = fopen(output, "a");
	if(!file_ptr){
		perror("Master: Error: 'clean_file()' failed at fopen()");
		exit(EXIT_FAILURE);
	}

	// Initialize shared memory
	if (init_shmry(6666) == -1){
		cleanup();
		exit(EXIT_FAILURE);
	}

	// Initialize message queue
	if ((msg_snd_id = init_mqueue(7777)) == -1){
		cleanup();
		exit(EXIT_FAILURE);
	}

	// Initialize message queue
	if ((msg_rcv_id = init_mqueue(8888)) == -1){
		cleanup();
		exit(EXIT_FAILURE);
	}

	shm_ptr[0].sec = 0;
	shm_ptr[0].nanosec = 0;

	// Initialize all process control blocks
	for( i = 0; i < PCBSIZE; i++){
		shm_ptr[0].p_block[i].active = 0;
		shm_ptr[0].p_block[i].time_used_sec = 0;
		shm_ptr[0].p_block[i].time_used_nanosec = 0;
		shm_ptr[0].p_block[i].local_pid = 0;
		shm_ptr[0].p_block[i].wait_sec = 0;
		shm_ptr[0].p_block[i].wait_nanosec = 0;
		shm_ptr[0].p_block[i].priority = 0;
		
	}

	// Initialize process tracker 0/1, NO/YES
	for ( i = 0; i < PCBSIZE; i++){
		p_tracker[i][0] = -1;
		p_tracker[i][1] = -1;
		p_tracker[i][2] = 0;
		p_tracker[i][3] = 0;
		p_tracker[i][4] = 0;
		p_tracker[i][5] = 0;
		p_tracker[i][6] = 0;
		p_tracker[i][7] = 0;
		p_tracker[i][8] = 0;
	}
	
	// String argument for child
	char exe_name[20];
	char exe_arg[10];
	strcpy(exe_name, "");
	strcpy(exe_name, "./dostuff");

	unsigned int nxtspawn_sec = get_randsec();
	unsigned int nxtspawn_nan = get_randnan();

	// Set up launch time
	shm_ptr[0].sec = nxtspawn_sec;
	shm_ptr[0].nanosec = nxtspawn_nan;
	total_processes = 1;

	// Fill process table entries
	p_tracker[0][0] = 0;
	p_tracker[0][1] = PRIOR_1;
	p_tracker[0][4] = shm_ptr[0].sec;
	p_tracker[0][5] = shm_ptr[0].nanosec;
	
	// Set up for initial fork
	local_pid = 0;
	shm_ptr[0].p_block[local_pid].local_pid = local_pid;
	shm_ptr[0].p_block[local_pid].active = 1;
	shm_ptr[0].p_block[local_pid].priority = PRIOR_1;
	enpqueue(p_queue[PRIOR_1], shm_ptr[0].p_block[local_pid]);	

	// Create message to let child run
	m_queue.mtype = local_pid + 1;
	m_queue.pc_block.t_quantum = QUANTUM[0];
	m_queue.pc_block.local_pid = local_pid;

	// Forking child initially
	child_pid[local_pid] = fork();
	if(child_pid[local_pid] == 0){
		sprintf(exe_arg, "%d", local_pid);
		char *args[] = { exe_name, exe_arg, NULL};
		execlp(args[0], args[1], args);
	}

	// Notify that a process is spawning
	fprintf(file_ptr, "OSS: Spawning process with PID %d process at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec );
	fprintf(file_ptr, "OSS: Process with PID %d enqueued to priority queue %d at %d:%d\n", local_pid, PRIOR_1, shm_ptr[0].sec, shm_ptr[0].nanosec);
	printf("OSS: Spawning process with PID %d process at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec );
	printf("OSS: Process with PID %d enqueued to priority queue %d at %d:%d\n", local_pid, PRIOR_1, shm_ptr[0].sec, shm_ptr[0].nanosec);

	// Get next spawn time
	nxtspawn_sec = get_randsec();
	nxtspawn_nan = get_randnan();

	// Increment clock while launching child processes
	while(total_processes <= PBOUND){

		// Check block processes
		for (i = 0; i < PRIOR; i++) {
			if ((p_node = depqueue(b_queue[i])) != NULL){
				if (i == PRIOR_1) {
					q1_counter++;
				}
				else if (i == PRIOR_2) {
					q2_counter++;
				}
				else if (i == PRIOR_3) {
					q3_counter++;
				}

				// Set data
				local_pid = p_node->block.local_pid;
				wait_sec = p_node->block.wait_sec;
				wait_nanosec = p_node->block.wait_nanosec;

				if (wait_sec <= shm_ptr[0].sec){
					if ((wait_nanosec <= shm_ptr[0].nanosec && wait_sec <= shm_ptr[0].sec) || (wait_sec < shm_ptr[0].sec)){
						p_tracker[local_pid][2] = 0; 
						p_tracker[local_pid][3] = 0;

						// Place in ready queue
						enpqueue(p_queue[i], p_node->block);
						fprintf(file_ptr, "OSS: Process with PID %d has received its resources and is leaving the block priority queue %d for ready priority queue %d at time %d:%d\n", local_pid, i, i, shm_ptr[0].sec, shm_ptr[0].nanosec);
						printf("OSS: Process with PID %d has received its resources and is leaving the block priority queue %d for ready priority queue %d at time %d:%d\n", local_pid, i, i, shm_ptr[0].sec, shm_ptr[0].nanosec);
						finish_wait = 1;
					}
				}
				if (finish_wait == -1){
					enpqueue(b_queue[i], p_node->block);
				}
				finish_wait = -1;
			}
		}

		// Dequeue process from highest priority queue
		for (i = 0; i < PRIOR; i++) {
			if ((p_node = depqueue(p_queue[i])) != NULL){
				
				local_pid = p_node->block.local_pid;
				fprintf(file_ptr, "OSS: Process with PID %d dequeued from priority queue %d at %d:%d\n", local_pid, i, shm_ptr[0].sec, shm_ptr[0].nanosec);
				printf("OSS: Process with PID %d dequeued from priority queue %d at %d:%d\n", local_pid, i, shm_ptr[0].sec, shm_ptr[0].nanosec);

				// Create message
				m_queue.mtype = local_pid + 1;
				m_queue.pc_block.t_quantum = QUANTUM[i];

				if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.pc_block), 0) == -1){
					perror("Master: processor(): msgsnd()");
					cleanup();
					return -1;
				}
				fprintf(file_ptr, "OSS: Message sent to process with PID %d at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
				printf("OSS: Message sent to process with PID %d at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);

				// Receiving message
				if (msgrcv(msg_rcv_id, &m_queue, sizeof(m_queue.pc_block), local_pid + 1, 0) == -1){
					perror("dostuff: msgrcv:");	
					return -1;
				}

				// Set data received
				child_option = m_queue.pc_block.child_option;
				used_sec = m_queue.pc_block.time_used_sec;
				used_nanosec = m_queue.pc_block.time_used_nanosec;
				wait_sec = m_queue.pc_block.wait_sec;
				wait_nanosec = m_queue.pc_block.wait_nanosec;
				active = m_queue.pc_block.active;

				// Increment clock accordingly
				shm_ptr[0].nanosec += used_nanosec;
				if ( shm_ptr[0].nanosec/SECOND == 1){
					shm_ptr[0].sec++;
					shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
				}

				switch (child_option) {
					case 0:
						// Child terminated
						fprintf(file_ptr, "OSS: Receiving that process with PID %d terminated at time %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
						printf("OSS: Receiving that process with PID %d terminated at time %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);

						// Record termination and reap child
						p_tracker[m_queue.pc_block.local_pid][0] = -1;
						p_tracker[m_queue.pc_block.local_pid][1] = -1;
						while(waitpid(0, &stat, WNOHANG) == 0);
						fprintf(file_ptr, "Process with PID %d properly reaped\n", m_queue.pc_block.local_pid);
						printf("Process with PID %d properly reaped\n", m_queue.pc_block.local_pid);
						break;
					case 1:
						// Child used entire time quantum
						fprintf(file_ptr, "OSS: Receiving that process with PID %d used its entire time quantum of %dns  at time %d:%d\n", local_pid, used_nanosec, shm_ptr[0].sec, shm_ptr[0].nanosec);
						printf("OSS: Receiving that process with PID %d used its entire time quantum of %dns  at time %d:%d\n", local_pid, used_nanosec, shm_ptr[0].sec, shm_ptr[0].nanosec);
						break;
					case 2:
						// Wait for something
						fprintf(file_ptr, "OSS: Receiving that process with PID %d is waiting for %d:%d at time %d:%d\n", local_pid, wait_sec, wait_nanosec, shm_ptr[0].sec, shm_ptr[0].nanosec);
						printf("OSS: Receiving that process with PID %d is waiting for %d:%d at time %d:%d\n", local_pid, wait_sec, wait_nanosec, shm_ptr[0].sec, shm_ptr[0].nanosec);

						// Update tracker
						p_tracker[m_queue.pc_block.local_pid][2] = wait_sec;
						p_tracker[m_queue.pc_block.local_pid][3] = wait_nanosec;

						// Enqueue to block queue
						enpqueue(b_queue[i], m_queue.pc_block);
						fprintf(file_ptr, "OSS: Process with PID %d is placed in block priority queue %d\n", local_pid, i);
						printf("OSS: Process with PID %d is placed in block priority queue %d\n", local_pid, i);
						break;
					case 3:
						fprintf(file_ptr, "OSS: Receiving that process with PID %d ran for %dns but was interrupted at time %d:%d\n", local_pid, used_nanosec, shm_ptr[0].sec, shm_ptr[0].nanosec);
						printf("OSS: Receiving that process with PID %d ran for %dns but was interrupted at time %d:%d\n", local_pid, used_nanosec, shm_ptr[0].sec, shm_ptr[0].nanosec);
						break;
				}
				current_priority = i;

				// Enqueue or not
				if (active == 1 && p_tracker[local_pid][2] <= 0 && p_tracker[local_pid][3] <= 0){
					/* Round robin scheduling [didnt have the heart to delete this]
					if (current_priority != 2)
						current_priority++;
					fprintf(file_ptr, "OSS: Process with PID %d enqueued to priority queue %d at time %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
					printf("OSS: Process with PID %d enqueued to priority queue %d at time %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
					enpqueue(p_queue[current_priority], shm_ptr[0].p_block[local_pid]);
					shm_ptr[0].p_block[local_pid].active = 0;
					*/

					// Process waited time
					waited_sec = shm_ptr[0].sec - p_tracker[local_pid][4] - used_sec;
					waited_nanosec = shm_ptr[0].nanosec - p_tracker[local_pid][5] - used_nanosec;
					if (waited_nanosec < 0){
						waited_sec--;
						waited_nanosec = waited_nanosec * -1;
						waited_nanosec = SECOND + waited_nanosec;
					}
					// Get total wait time of this process
					fprintf(file_ptr, "OSS: Process waited time %d:%d\n", waited_sec, waited_nanosec);
					printf("OSS: Process waited time %d:%d\n", waited_sec, waited_nanosec);

					if (current_priority != 2) {
						q_counter = 0;
						waited_sec_sum = 0;
						waited_nanosec_sum = 0;

						// Sum wait time of process in 1 level lower in priority
						for (i = 0; i < PCBSIZE; i++) {
							if (p_tracker[i][1] == current_priority + 1) {
								waited_sec_sum += p_tracker[i][6];
								waited_nanosec_sum += p_tracker[i][7];
								q_counter++;
							}
						}

						// Get average wait time
						if (q_counter != 0){
							avg_wait_sec = waited_sec_sum/q_counter;
							avg_wait_nanosec = waited_nanosec_sum/q_counter;
						}
						else{
							avg_wait_sec = waited_sec_sum;
							avg_wait_nanosec = waited_nanosec_sum;
						}
						fprintf(file_ptr, "OSS: Average wait time in priority queue %d is %d:%d\n", current_priority + 1, avg_wait_sec, avg_wait_nanosec);
						printf("OSS: Average wait time in priority queue %d is %d:%d\n", current_priority + 1, avg_wait_sec, avg_wait_nanosec);

						// Determine where to enqueue
						if (waited_sec > S_WBOUND || (waited_sec == S_WBOUND && waited_nanosec > NS_WBOUND)) {
							if (waited_sec > CONSTANT[current_priority] * avg_wait_sec || (waited_sec == CONSTANT[current_priority] * avg_wait_sec && waited_nanosec > CONSTANT[current_priority] * avg_wait_nanosec)){
								fprintf(file_ptr, "OSS: Process with PID %d enqueued to queue %d at %d:%d\n", local_pid, current_priority + 1, shm_ptr[0].sec, shm_ptr[0].nanosec);
								printf("OSS: Process with PID %d enqueued to queue %d at %d:%d\n", local_pid, current_priority + 1, shm_ptr[0].sec, shm_ptr[0].nanosec);
								// Enqueuing to lower priority
								enpqueue(p_queue[current_priority + 1], shm_ptr[0].p_block[local_pid]);
								p_tracker[local_pid][1]++;
								p_tracker[local_pid][8] = 0;
							}
							else{
								// Enqueue bound: move to lower priority if process enqueue bound reached
								if (p_tracker[local_pid][8] >= QBOUND) {
									fprintf(file_ptr, "OSS: Process with PID %d has reached the enqueue limit (%d) for priority queue %d\n", local_pid, QBOUND, current_priority);
									fprintf(file_ptr, "OSS: Process with PID %d enqueued to queue %d at %d:%d\n", local_pid, current_priority + 1, shm_ptr[0].sec, shm_ptr[0].nanosec);
									printf("OSS: Process with PID %d has reached the enqueue limit (%d) for priority queue %d\n", local_pid, QBOUND, current_priority);
									printf("OSS: Process with PID %d enqueued to queue %d at %d:%d\n", local_pid, current_priority + 1, shm_ptr[0].sec, shm_ptr[0].nanosec);
									enpqueue(p_queue[current_priority + 1], shm_ptr[0].p_block[local_pid]);
									p_tracker[local_pid][1]++;
									p_tracker[local_pid][8] = 0;
								}
								else{
									fprintf(file_ptr, "OSS: Process with PID %d moved to the end of its current queue %d at %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
									printf("OSS: Process with PID %d moved to the end of its current queue %d at %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
									// Enqueue to same priority queue
									enpqueue(p_queue[current_priority], shm_ptr[0].p_block[local_pid]);
									p_tracker[local_pid][8]++;
								}
							}
						}
						else{
							// Enforce enqueue bound
							if (p_tracker[local_pid][8] >= QBOUND) {
								fprintf(file_ptr, "OSS: Process with PID %d has reached the enqueue limit (%d) for priority queue %d\n", local_pid, QBOUND, current_priority);
								fprintf(file_ptr, "OSS: Process with PID %d enqueued to queue %d at %d:%d\n", local_pid, current_priority + 1, shm_ptr[0].sec, shm_ptr[0].nanosec);
								printf("OSS: Process with PID %d has reached the enqueue limit (%d) for priority queue %d\n", local_pid, QBOUND, current_priority);
								printf("OSS: Process with PID %d enqueued to queue %d at %d:%d\n", local_pid, current_priority + 1, shm_ptr[0].sec, shm_ptr[0].nanosec);
								enpqueue(p_queue[current_priority + 1], shm_ptr[0].p_block[local_pid]);
								p_tracker[local_pid][1]++;
								p_tracker[local_pid][8] = 0;
							}
							else{
								fprintf(file_ptr, "OSS: Process with PID %d moved to the end of its current queue %d at %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
								printf("OSS: Process with PID %d moved to the end of its current queue %d at %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
								enpqueue(p_queue[current_priority], shm_ptr[0].p_block[local_pid]);
								p_tracker[local_pid][8]++;
							}
						}
					}
					else{
						// If process is already at lowest priority
						fprintf(file_ptr, "OSS: Process with PID %d moved to the end of its current queue %d at %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
						printf("OSS: Process with PID %d moved to the end of its current queue %d at %d:%d\n", local_pid, current_priority, shm_ptr[0].sec, shm_ptr[0].nanosec);
						enpqueue(p_queue[current_priority], shm_ptr[0].p_block[local_pid]);
						p_tracker[local_pid][8]++;
					}

					// Update process control table
					p_tracker[local_pid][4] = shm_ptr[0].sec;
					p_tracker[local_pid][5] = shm_ptr[0].nanosec;
					p_tracker[local_pid][6] = waited_sec;
					p_tracker[local_pid][7] = waited_nanosec;
					shm_ptr[0].p_block[local_pid].active = 0;
				}

				// Randomize dispatch time
				dispatch_time = rand() % D_UBOUND + D_LBOUND;
				fprintf(file_ptr, "OSS: Total time spent in dispatcher %dns\n", dispatch_time);
				printf("OSS: Total time spent in dispatcher %dns\n", dispatch_time);

				// Increment clock according to dispatch time
				shm_ptr[0].nanosec += dispatch_time;
				if ( shm_ptr[0].nanosec/SECOND == 1){
					shm_ptr[0].sec++;
					shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
				}
				break;
			}
		}
		
		// Check if bound limit reached
		if (total_processes < PBOUND){
			if (nxtspawn_sec <= shm_ptr[0].sec ){
				
				// Get index of available block
				index = has_opening(p_tracker, PCBSIZE);
				
				if ((nxtspawn_nan <= shm_ptr[0].nanosec && index >= 0) || (nxtspawn_sec < shm_ptr[0].sec && index >= 0)){
					fprintf(file_ptr, "OSS: Spawning process with PID %d at %d:%d\n", index, shm_ptr[0].sec, shm_ptr[0].nanosec );
					printf("OSS: Spawning process with PID %d at %d:%d\n", index, shm_ptr[0].sec, shm_ptr[0].nanosec );
					nxtspawn_sec = shm_ptr[0].sec + get_randsec();
					nxtspawn_nan = shm_ptr[0].nanosec + get_randnan();
					if (nxtspawn_nan/SECOND == 1){
						nxtspawn_sec++;
						nxtspawn_nan = nxtspawn_nan % SECOND;
					}
					// Set up initial block for process table
					p_tracker[index][0] = index;
					p_tracker[index][1] = PRIOR_1;
					p_tracker[index][2] = 0;
					p_tracker[index][3] = 0;
					p_tracker[index][4] = shm_ptr[0].sec;
					p_tracker[index][5] = shm_ptr[0].nanosec;
					p_tracker[index][6] = 0;
					p_tracker[index][7] = 0;
					total_processes++;
					shm_ptr[0].p_block[index].local_pid = index;
					fprintf(file_ptr, "Total processes active = %d\n", total_running(p_tracker, PCBSIZE));
					fprintf(file_ptr, "Total processes spawned = %d\n", total_processes);
					printf("Total processes active = %d\n", total_running(p_tracker, PCBSIZE));
					printf("Total processes spawned = %d\n", total_processes);

					// Forking child
					enpqueue(p_queue[PRIOR_1], shm_ptr[0].p_block[index]);
					fprintf(file_ptr, "OSS: Process with PID %d enqueued to priority queue %d at time %d:%d\n", index, PRIOR_1, shm_ptr[0].sec, shm_ptr[0].nanosec);
					printf("OSS: Process with PID %d enqueued to priority queue %d at time %d:%d\n", index, PRIOR_1, shm_ptr[0].sec, shm_ptr[0].nanosec);
					child_pid[index] = fork();
					if(child_pid[index] == 0){
						sprintf(exe_arg, "%d", index);
						char *args[] = { exe_name, exe_arg, NULL};
						execlp(args[0], args[1], args);
					}
				}
			}
		}

		// Let the process that broke/matches PBOUND execute if time permits
		if (total_processes == PBOUND)
			finish = 1;
		for (i = 0; i < PRIOR; i++) {
			if (total_processes == PBOUND && p_queue[i]->front != NULL){
				finish = -1;
				break;
			}
		}
		if (finish == 1){
			break;
		}
		shm_ptr[0].nanosec += NANOSEC;	
		if ( shm_ptr[0].nanosec/SECOND == 1){
			shm_ptr[0].sec++;
			shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
		}
	}	

	for (i = 0; i < PCBSIZE; i++){
		if (p_tracker[i][0] > -1){
			kill(child_pid[i], SIGKILL);
		}
	}
	fprintf(file_ptr, "Queue 1 dequeued total = %d\n", q1_counter);
	fprintf(file_ptr, "Queue 2 dequeued total = %d\n", q2_counter);
	fprintf(file_ptr, "Queue 3 dequeued total = %d\n", q3_counter);
	fprintf(file_ptr, "Total of %d program executed in %d:%d\n", total_processes, shm_ptr[0].sec, shm_ptr[0].nanosec);

	printf("Log file '%s' generated.\n", output);

	printf("Queue 1 dequeued total = %d\n", q1_counter);
	printf("Queue 2 dequeued total = %d\n", q2_counter);
	printf("Queue 3 dequeued total = %d\n", q3_counter);
	printf("Total of %d program executed in %d:%d\n", total_processes, shm_ptr[0].sec, shm_ptr[0].nanosec);

	// Detach/closes any opening resources
	cleanup();
	return 0;
}

// Kill all children
static void sig_handler(int s){
	int i = 0;
	int stat;
	switch(s){
		// Handle alarm from real timer
		case SIGALRM:
			printf("%d seconds of real time has passed. Terminating...\n", TIMER);
			for( i = 0; i < PCBSIZE; i++){
				if(kill(child_pid[i], SIGKILL) == 0){
					while(waitpid(-1, &stat, WNOHANG)== 0);
					printf("Master: %d killed from SIGALARM\n", child_pid[i]);
				}
			}
			cleanup();
			break;
		// Handle anyother interuptions
		case SIGINT:
			for( i = 0; i < PCBSIZE; i++){
				if(kill(child_pid[i], SIGKILL) == 0){
					while(waitpid(-1, &stat, WNOHANG)== 0);
					printf("Master: %d killed from SIGINT\n", child_pid[i]);
				}
			}
			cleanup();
			break;
	}
	exit(0);
}

// Cleanup function
void cleanup(){
	int i;
	for (i = 0; i < PRIOR; i++) {
		free(p_queue[i]);
	}
	for (i = 0; i < PRIOR; i++) {
		free(b_queue[i]);
	}
	if(shmdt(shm_ptr) == -1){
		perror("Master: cleanup(): shmdt():");
	}
	shmctl(shm_id, IPC_RMID, NULL);
	msgctl(msg_snd_id, IPC_RMID, NULL);
	msgctl(msg_rcv_id, IPC_RMID, NULL);
	if (file_ptr != NULL){
		fclose(file_ptr);
	}
	printf("Clean Complete\n");
}
