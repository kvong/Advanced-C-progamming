#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "shmry.h"
#include <spawnclock.h>
#include <rqueue.h>
#include <sys/msg.h>
#include <msgqueue.h>

/******************************************************************************
* File:             master.c
*
* Author:           Khanh Vong  
* Created:          11/10/19 
* Description:      OS multi-process deadlock simulator; Operate on Deadlock 
*					Avoidance (DLA) Algorithm; 15-25% of resources are shareable;
*					IPC are done via shared memory and message queue.
*****************************************************************************/


// CONSTANTS
#define CHILDREN 18
#define RSRCSIZE 20
#define PBOUND 100

// Loose ends to take care of at termination
FILE* file_ptr = NULL;
pid_t child_pid[CHILDREN];
static shmry_t* shm_ptr;
static msgqueue_t m_queue;
int shm_id, msg_snd_id, msg_rcv_id;
int p_tracker[CHILDREN];
queue_r* wait_queue = NULL;
queue_r* active_queue = NULL;


// Default conditions for oss
const int real_time = 2;

// Prototypes
void clean_file(char*);
int processor(char*, int);
static void sig_handler(int);
void cleanup();

int init_shmry(key_t);
int init_mqueue(key_t);

int has_running(int [CHILDREN], int);
int has_opening(int [CHILDREN], int);
int new_turn(int [CHILDREN], int , int);

int main(int argc, char** argv){
	int shareable_flag;
	//Set up master alarm
	signal(SIGALRM, (void (*)(int))sig_handler);
	signal(SIGINT, (void (*)(int))sig_handler);
	alarm(real_time);

	// Defaut outputfile will be called oss.log
	char output[20] = "oss.log";

	if (argc > 1) {
		shareable_flag = 0;
	}
	else
		shareable_flag = 1;

	// Manage processes
	processor(output, shareable_flag);
	perror("Program exit");
	return 0;
}

// Clean file for writing
void clean_file(char* input){
	file_ptr = fopen(input, "w");
	if(!file_ptr){
		printf("File '%d' could not be opened for cleaning.\n", input);
		perror("Master: Error: 'clean_file()' failed at fopen()");
		exit(EXIT_FAILURE);
	}
	fclose(file_ptr);
}

// Initialize shared memory global
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

// Initialize message queue global
int init_mqueue(key_t key){
	int msgid;
	if((msgid = msgget(key, IPC_CREAT | PERMS)) < 0){
		perror("Master: processor(): msgget:");
		return -1;
	}
	return msgid;
}

// Check block for active processes
int has_running(int block[CHILDREN], int size){
	int i;
	for (i = 0; i < size; i++) {
		if (block[i] != -1) {
			return i;
		}
	}
	return -1;
}

// Check openings to launch process
int has_opening(int block[CHILDREN], int size){
	int i;
	for (i = 0; i < size; i++) {
		if (block[i] == -1) {
			return i;
		}
	}
	return -1;
}

// Simulate os
int processor(char* output, int sh_flag){
	// For descriptor
	int shareable;
	int sh_count = 0;
	int i, j, k, index;
	int queue_size;

	// For Forking
	int stat;
	int total_processes = 0;
	int terminate_total = 0;
	pid_t pid;
	int nxt_spawn_sec;
	int nxt_spawn_nan;

	// For requests
	int local_pid, option, r_type, r_amount, r_localpid;
	int r_available[20];
	int r_need[20];
	int alloc_total[20];
	int unsafe;
	int total_unsafe = 0;
	int total_granted = 0;

	int dispatch_time;
	int total_lines = 0;

	// For stats
	int total_blocked = 0;
	int total_waits = 0;
	int total_deadlock = 0;
	int total_reaped = 0;
	int avg_run_sec = 0;
	int avg_run_nanosec = 0;
	int avg_wait_sec = 0;
	int avg_wait_nanosec = 0;

	// For process queue
	request_t tmp;
	node_r* r_node = NULL;
	wait_queue = init_rqueue();
	active_queue = init_rqueue();

	srand(time(NULL));
	
	// Clean file for writing
	clean_file(output);

	// Open file for appending
	file_ptr = fopen(output, "a");
	if(!file_ptr){
		perror("oss: Error: 'clean_file()' failed at fopen()");
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

	// Set up descriptor
	for (i = 0; i < RSRCSIZE; i++) {
		shm_ptr[0].descriptor[i].resource = rand() % 10 + 1;
		shareable = rand() % 100 + 1;
		shareable = shareable % 5;
		if (shareable == 0 && sh_count < 5){
			shm_ptr[0].descriptor[i].shareable = 1;
			sh_count++;	
		}
		else
			shm_ptr[0].descriptor[i].shareable = 0;
	}

	// Set descriptor to have atleast 15% shareable at all time
	if (sh_count < 3){
		for (i = sh_count; i < 3; i++) {
			shm_ptr[0].descriptor[rand() % 19].shareable = 1;
			sh_count++;
		}
	}

	if (sh_flag == 0) {
		for (i = 0; i < RSRCSIZE; i++) {
			shm_ptr[0].descriptor[i].shareable = 0;
		}
	}

	fprintf(file_ptr, "Master: Shared Descriptor initialized\n");
	total_lines++;
	// Print resource descriptor; This is read-only
	for (i = 0; i < RSRCSIZE; i++) {
		if (shm_ptr[0].descriptor[i].shareable == 1) {
			fprintf(file_ptr, " R%d = %dsh |", i, shm_ptr[0].descriptor[i].resource);
		}
		else
			fprintf(file_ptr, " R%d = %d   |", i, shm_ptr[0].descriptor[i].resource);
	}
	fprintf(file_ptr, "\n");
	total_lines++;

	// Set up max claim table
	for (i = 0; i < CHILDREN; i++) {
		for (j = 0; j < RSRCSIZE; j++) {
			shm_ptr[0].max_claim[i][j] = 0;
			shm_ptr[0].r_allocation[i][j] = 0;
		}
	}

	
	// Set up process tracker
	for (i = 0; i < CHILDREN; i++) {
		p_tracker[i] = -1;
	}

	// String argument for child
	char exe_name[20];
	char exe_arg[10];
	strcpy(exe_name, "");
	strcpy(exe_name, "./dostuff");

	nxt_spawn_nan = get_randnan();
	nxt_spawn_sec = get_randsec();
	shm_ptr[0].sec = nxt_spawn_sec;
	shm_ptr[0].nanosec = nxt_spawn_nan;

	// Child execute dostuff
	p_tracker[0] = 1;
	total_processes++;

	tmp.local_pid = 0;
	tmp.option = -1;
	tmp.r_type = 0;
	tmp.r_amount = 0;
	
	// Put newly created process into queue
	enrqueue(active_queue, tmp);
	fprintf(file_ptr, "Master: Process with PID %d created at %d:%d.\n", 0, shm_ptr[0].sec, shm_ptr[0].nanosec);
	total_lines++;

	dispatch_time = rand() % 30 + 20;
	shm_ptr[0].nanosec += dispatch_time;
	if ( shm_ptr[0].nanosec/SECOND == 1){
		shm_ptr[0].sec++;
		shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
	}

	fprintf(file_ptr, "Master: PID %d placed into active_queue at %d:%d\n", 0, shm_ptr[0].sec, shm_ptr[0].nanosec);
	total_lines++;

	child_pid[0] = fork();
	if (child_pid[0] == 0){
		// Add an additional commandline argument to keep track of which child has exitted
		sprintf(exe_arg, "%d", 0);
		char *args[] = { exe_name, exe_arg, NULL};
		execlp(args[0], args[1], args);
	}

	nxt_spawn_nan = get_randnan() + shm_ptr[0].nanosec;
	nxt_spawn_sec = get_randsec() + shm_ptr[0].sec;


	fprintf(file_ptr, "Master: Next spawn time is %d:%d\n", nxt_spawn_sec, nxt_spawn_nan);
	total_lines++;
	while (total_processes < PBOUND){
		// Calculate allocated total
		for (i = 0; i < RSRCSIZE; i++) {
			alloc_total[i] = 0;
			for (j = 0; j < CHILDREN; j++) {
				alloc_total[i] += shm_ptr[0].r_allocation[j][i];
			}
		}

		// Calculate available resources
		for (i = 0; i < RSRCSIZE; i++) {
			r_available[i] = 0;
			r_available[i] += (shm_ptr[0].descriptor[i].resource - alloc_total[i]);
		}

		// Process wait queue first
		total_unsafe = 0;
		queue_size = wait_queue->size;
		if (queue_size > 0) {
			if (total_lines <= 100000){
				fprintf(file_ptr, "Master: Accessing wait_queue at %d:%d\n", shm_ptr[0].sec, shm_ptr[0].nanosec);
				total_lines++;	
			}
			for (i = 0; i < queue_size; i++) {
				// Calculate allocated total
				for (j = 0; j < RSRCSIZE; j++) {
					alloc_total[j] = 0;
					for (k = 0; k < CHILDREN; k++) {
						alloc_total[j] += shm_ptr[0].r_allocation[k][j];
					}
				}

				// Calculate available resources
				for (j = 0; j < RSRCSIZE; j++) {
					r_available[j] = 0;
					r_available[j] += (shm_ptr[0].descriptor[j].resource - alloc_total[j]);
				}

				r_node = derqueue(wait_queue);
				local_pid = r_node->request.local_pid;
				r_amount = r_node->request.r_amount;
				r_type = r_node->request.r_type;

				// Check if resource is shareable
				if (shm_ptr[0].descriptor[r_type].shareable) {
					// Don't perform DLA on shareable resources
					if (r_available[r_type] >= r_amount) {
						if (total_lines <= 100000){
							fprintf(file_ptr, "Master: PID %d request for %d of shareable R%d is granted at %d:%d\n", local_pid, r_amount, r_type, shm_ptr[0].sec, shm_ptr[0].nanosec);
							total_lines++;
						}
						shm_ptr[0].r_allocation[local_pid][r_type] += r_amount;
						enrqueue(active_queue, r_node->request);

						avg_wait_sec += (shm_ptr[0].sec - r_node->request.wait_sec);
						avg_wait_nanosec += abs(shm_ptr[0].nanosec - r_node->request.wait_nanosec);
						total_waits++;

						total_granted++;
						if (total_granted%20 == 0) {
							if (total_lines <= 100000){
								fprintf(file_ptr, "Master: 20 Request has been granted:\n");
								total_lines++;
								fprintf(file_ptr, "Current Allocation Table:\n");
								total_lines++;
								for (j = 0; j < CHILDREN; j++) {
									fprintf(file_ptr, "PID %d:", j);
									for (k = 0; k < RSRCSIZE; k++) {
										fprintf(file_ptr, " %d |", shm_ptr[0].r_allocation[j][k]);	
									}
									fprintf(file_ptr, "\n");
									total_lines++;
								}
							}
						}
					}
					else{
						if (total_lines <= 100000){
							fprintf(file_ptr, "Master: PID %d request for %d of shareable R%d is denied at %d:%d\n", local_pid, r_amount, r_type, shm_ptr[0].sec, shm_ptr[0].nanosec);
							total_lines++;
							fprintf(file_ptr, "Available table:\n");
							total_lines++;
							for (j = 0; j < RSRCSIZE; j++) {
								fprintf(file_ptr, " %d| ", r_available[j]);
							}
							fprintf(file_ptr, "\nNeed table for PID %d:\n", local_pid);
							total_lines++;
							total_lines++;
							for (j = 0; j < RSRCSIZE; j++) {
								fprintf(file_ptr, " %d |", r_need[j]);
							}
							fprintf(file_ptr, "\n");
							total_lines++;
						}
						total_unsafe++;
						enrqueue(wait_queue, r_node->request);
					}
				}
				else{
					// Perform DLA 
					for (j = 0; j < RSRCSIZE; j++) {
						r_need[j] = shm_ptr[0].max_claim[local_pid][j] - shm_ptr[0].r_allocation[local_pid][j];
					}
					
					unsafe = 0;
					for (j = 0; j < RSRCSIZE; j++) {
						if (r_need[j] > r_available[j] && shm_ptr[0].descriptor[j].shareable == 0) {
							unsafe = 1;
							break;
						}
					}
					
					// Process base on safe-state
					if (unsafe == 0) {
						// On safe states
						if (r_amount == 0 && r_type == 0) {
							
							if (total_lines <= 100000) {
								fprintf(file_ptr, "Master: PID %d has no resources to release at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
								total_lines++;
							}
						}
						else{
							if (total_lines <= 100000) {
								fprintf(file_ptr, "Master: PID %d granted %d of R%d at %d:%d\n", local_pid, r_amount, r_type, shm_ptr[0].sec, shm_ptr[0].nanosec);
								total_lines++;
							}
							shm_ptr[0].r_allocation[local_pid][r_type] += r_amount;
						}
						// If resource is granted, it is no longer blocked
						enrqueue(active_queue, r_node->request);

						avg_wait_sec += (shm_ptr[0].sec - r_node->request.wait_sec);
						avg_wait_nanosec += abs(shm_ptr[0].nanosec - r_node->request.wait_nanosec);
						total_waits++;

						total_granted++;
						if (total_granted%20 == 0) {
							if (total_lines <= 100000) {
								fprintf(file_ptr, "Master: 20 Request has been granted:\n");
								fprintf(file_ptr, "Current Allocation Table:\n");
								for (j = 0; j < CHILDREN; j++) {
									fprintf(file_ptr, "PID %d:", j);
									for (k = 0; k < RSRCSIZE; k++) {
										fprintf(file_ptr, " %d |", shm_ptr[0].r_allocation[j][k]);	
									}
									fprintf(file_ptr, "\n");
								}
							}
						}
					}
					else{
						if (total_lines <= 100000){
							// Unsafe states
							fprintf(file_ptr, "Master: Unsafe state detected for Child PID %d at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
							total_lines++;
							fprintf(file_ptr, "Available table:\n");
							total_lines++;
							for (j = 0; j < RSRCSIZE; j++) {
								fprintf(file_ptr, " %d| ", r_available[j]);
							}
							fprintf(file_ptr, "\nNeed table for PID %d:\n", local_pid);
							total_lines++;
							total_lines++;
							for (j = 0; j < RSRCSIZE; j++) {
								fprintf(file_ptr, " %d |", r_need[j]);
							}
							fprintf(file_ptr, "\n");
							total_lines++;
						}
						total_unsafe++;
						// Put process at the end of the wait queue
						enrqueue(wait_queue, r_node->request);
					}
				}
				dispatch_time = rand() % 20 + 10;
				shm_ptr[0].nanosec += dispatch_time;
				if ( shm_ptr[0].nanosec/SECOND == 1){
					shm_ptr[0].sec++;
					shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
				}
			}
		}

		// If none of the blocked processes were granted their resources or if theres no wait queue
		if ((total_unsafe == wait_queue->size && queue_size != 0) || queue_size < 1 ) {
			if (total_lines <= 100000){
				fprintf(file_ptr, "Master: Accessing active_queue at %d:%d\n", shm_ptr[0].sec, shm_ptr[0].nanosec);
				total_lines++;
			}

			queue_size = active_queue->size;
			if (queue_size > 0) {
				// Go through all node in active queue
				for (i = 0; i < queue_size; i++) {
					// Calculate allocated total
					for (j = 0; j < RSRCSIZE; j++) {
						alloc_total[j] = 0;
						for (k = 0; k < CHILDREN; k++) {
							alloc_total[j] += shm_ptr[0].r_allocation[k][j];
						}
					}

					// Calculate available resources
					for (j = 0; j < RSRCSIZE; j++) {
						r_available[j] = 0;
						r_available[j] += (shm_ptr[0].descriptor[j].resource - alloc_total[j]);
					}
					r_node = derqueue(active_queue);
					local_pid = r_node->request.local_pid;
					m_queue.mtype = local_pid +1;
					m_queue.msg = r_node->request;
					// Send run message
					if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0) == -1){
						perror("Master: processor(): msgsnd()");
						cleanup();
						return -1;
					}
					if (msgrcv(msg_rcv_id, &m_queue, sizeof(m_queue.msg), local_pid + 1, 0) == -1){
						perror("dostuff: msgrcv:");	
						return -1;
					}
					option = m_queue.msg.option;
					r_type = m_queue.msg.r_type;
					r_amount = m_queue.msg.r_amount;
					// Procesing childs option
					switch (option) {
						case 0:
							// Creating need array.
							for (j = 0; j < RSRCSIZE; j++) {
								r_need[j] = shm_ptr[0].max_claim[local_pid][j] - shm_ptr[0].r_allocation[local_pid][j];
							}
							// Processing shareable data
							if (shm_ptr[0].descriptor[r_type].shareable) {
								if (total_lines <= 100000) {
									fprintf(file_ptr, "Master: PID %d requested a shareable resource at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
									total_lines++;
								}
								if (r_available[r_type] >= r_amount) {
									if (total_lines <= 100000) {
										fprintf(file_ptr, "Master: PID %d request for %d of shareable R%d was approved\n", local_pid, r_amount, r_type);
										total_lines++;
									}
									shm_ptr[0].r_allocation[local_pid][r_type] += r_amount;
									enrqueue(active_queue, m_queue.msg);
									total_granted++;
									if (total_granted%20 == 0 && total_lines <= 100000) {
										fprintf(file_ptr, "Master: 20 Request has been granted:\n");
										fprintf(file_ptr, "Current Allocation Table:\n");
										total_lines++;
										total_lines++;
										for (j = 0; j < CHILDREN; j++) {
											fprintf(file_ptr, "PID %d:", j);
											for (k = 0; k < RSRCSIZE; k++) {
												fprintf(file_ptr, " %d |", shm_ptr[0].r_allocation[j][k]);	
											}
											fprintf(file_ptr, "\n");
											total_lines++;
										}
									}
								}
								else{
									if (total_lines <=  100000) {
										fprintf(file_ptr, "Master: PID %d request for %d of shareable R%d was denied\n", local_pid, r_amount, r_type);
										fprintf(file_ptr, "Available table:\n");
										total_lines++;
										total_lines++;
										for (j = 0; j < RSRCSIZE; j++) {
											fprintf(file_ptr, " %d| ", r_available[j]);
										}
										fprintf(file_ptr, "\nNeed table for PID %d:\n", local_pid);
										total_lines++;
										total_lines++;
										for (j = 0; j < RSRCSIZE; j++) {
											fprintf(file_ptr, " %d |", r_need[j]);
										}
										fprintf(file_ptr, "\n");
										total_lines++;
									}
									m_queue.msg.wait_sec = shm_ptr[0].sec;
									m_queue.msg.wait_nanosec = shm_ptr[0].nanosec;
									enrqueue(wait_queue, m_queue.msg);
									total_blocked++;
								}
							}
							else{
								if (total_lines <= 100000) {
									fprintf(file_ptr, "Master: PID %d requested a resource at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
									total_lines++;
									
								}
								// Deadlock Avoidance (ignore shareable resources)
								unsafe = 0;
								for (j = 0; j < RSRCSIZE; j++) {
									if (r_need[j] > r_available[j] && shm_ptr[0].descriptor[j].shareable == 0) {
										unsafe = 1;
										break;
									}
								}
								if (unsafe == 0) {
									if (r_amount == 0 && r_type == 0) {
										if (total_lines <= 100000) {
											fprintf(file_ptr, "Master: PID %d max_claim reached.\n", local_pid);
											total_lines++;
										}
									}
									else{
										if (total_lines <= 100000) {
											fprintf(file_ptr, "Master: Conclude that it is safe to grant PID %d %d of resouce R%d.\n", local_pid, r_amount, r_type);
											total_lines++;
										}
										shm_ptr[0].r_allocation[local_pid][r_type] += r_amount;
									}
									// Granted put in active queue
									enrqueue(active_queue, m_queue.msg);
									total_granted++;
									if (total_granted%20 == 0 && total_lines <= 100000) {
										fprintf(file_ptr, "Master: 20 Request has been granted:\n");
										fprintf(file_ptr, "Current Allocation Table:\n");
										total_lines++;
										total_lines++;
										for (j = 0; j < CHILDREN; j++) {
											fprintf(file_ptr, "PID %d:", j);
											for (k = 0; k < RSRCSIZE; k++) {
												fprintf(file_ptr, " %d |", shm_ptr[0].r_allocation[j][k]);	
											}
											fprintf(file_ptr, "\n");
											total_lines++;
										}
									}
								}
								else{
									if (total_lines <= 100000) {
										fprintf(file_ptr, "Master: Conclude that it is unsafe to grant PID %d %d of resouce R%d.\n", local_pid, r_amount, r_type);
										fprintf(file_ptr, "Available table:\n");
										total_lines++;
										total_lines++;
										for (j = 0; j < RSRCSIZE; j++) {
											fprintf(file_ptr, " %d| ", r_available[j]);
										}
										fprintf(file_ptr, "\nNeed table for PID %d:\n", local_pid);
										total_lines++;
										for (j = 0; j < RSRCSIZE; j++) {
											fprintf(file_ptr, " %d |", r_need[j]);
										}
										fprintf(file_ptr, "\n");
										total_lines++;
									}
									// Not granted put in wait/block queue
									m_queue.msg.wait_sec = shm_ptr[0].sec;
									m_queue.msg.wait_nanosec = shm_ptr[0].nanosec;
									enrqueue(wait_queue, m_queue.msg);
									total_blocked++;
								}
							}
							break;
						case 1:
							if (total_lines <= 100000) {
								fprintf(file_ptr, "Master: PID %d released some of its resource\n", local_pid);
								total_lines++;
								
							}
							// Releasing resources
							if (r_amount == 0 && r_type == 0) {
								if (total_lines <= 100000) {
									// If theres no resources to release
									fprintf(file_ptr, "Master: PID %d have no resources to release.\n", local_pid);
									total_lines++;
								}
							}
							else{
								if (total_lines <= 100000) {
									fprintf(file_ptr, "Master: PID %d released %d of R%d.\n", local_pid, r_amount, r_type);
									total_lines++;
								}
								shm_ptr[0].r_allocation[local_pid][r_type] -= r_amount;
							}
							// Put in back of active queue
							enrqueue(active_queue, m_queue.msg);
							break;
						case 2:
							// Termination
							if (total_lines <= 100000) {
								fprintf(file_ptr, "Master: PID %d terminated at %d:%d.\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
								total_lines++;
							}
							wait(&stat);
							avg_run_sec += (shm_ptr[0].sec - m_queue.msg.start_sec);
							avg_run_nanosec += abs(shm_ptr[0].nanosec - m_queue.msg.start_nanosec);
							for (j = 0; j < RSRCSIZE; j++) {
								shm_ptr[0].r_allocation[local_pid][j] = 0;
								shm_ptr[0].max_claim[local_pid][j] = 0;
							}
							p_tracker[local_pid] = -1;
							total_reaped++;
							break;
					}
					dispatch_time = rand() % 20 + 10;
					shm_ptr[0].nanosec += dispatch_time;
					if ( shm_ptr[0].nanosec/SECOND == 1){
						shm_ptr[0].sec++;
						shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
					}
				}
			}
		}

		if (wait_queue->size == 18){
			total_deadlock++;
			if (total_lines <= 100000) {
				fprintf(file_ptr, "Master: System Deadlocked\n");
				fprintf(file_ptr, "Master: Initiating Deadlock Recovery\n");
				total_lines++;
				total_lines++;
				for (i = 0; i < CHILDREN; i++) {
					for (j = 0; j < RSRCSIZE; j++) {
						shm_ptr[0].r_allocation[i][j] = 0;
					}
				}
			}
		}

		// Process spawner
		if (total_processes < PBOUND){
			if (nxt_spawn_sec <= shm_ptr[0].sec){
				index = has_opening(p_tracker, CHILDREN);

				// Launch only when theres an opening on the tracker
				if ((nxt_spawn_nan <= shm_ptr[0].nanosec && index >= 0) || (nxt_spawn_sec < shm_ptr[0].sec && index >= 0)) {
					fprintf(file_ptr, "Master: Launching new process with PID %d at %d:%d\n", index, shm_ptr[0].sec, shm_ptr[0].nanosec);
					total_processes++;

					// Get next spawn time
					nxt_spawn_sec = shm_ptr[0].sec + get_randsec();
					nxt_spawn_nan = shm_ptr[0].nanosec + get_randnan();
					if (nxt_spawn_nan/SECOND == 1) {
						nxt_spawn_sec++;
						nxt_spawn_nan = nxt_spawn_nan % SECOND;
					}
					if (total_lines <= 100000){
						fprintf(file_ptr, "Master: Total processes created = %d\n", total_processes);
						if (total_processes != PBOUND) {
							fprintf(file_ptr, "Master: Next spawn time is %d:%d.\n", nxt_spawn_sec, nxt_spawn_nan);
						}
						total_lines += 2;
					}
					
					// Update reacker and put into active queue
					p_tracker[index] = 1;
					tmp.local_pid = index;
					tmp.option = -1;
					tmp.r_type = 0;
					tmp.r_amount = 0;
					enrqueue(active_queue, tmp);
					child_pid[index] = fork();
					if (child_pid[index] == 0) {
						// Create string to exec
						sprintf(exe_arg, "%d", index);
						char *args[] = { exe_name, exe_arg, NULL};
						execlp(args[0], args[1], args);
					}
				}
			}
		}

		// Increment clock
		shm_ptr[0].nanosec += 10;	
		if ( shm_ptr[0].nanosec/SECOND == 1){
			shm_ptr[0].sec++;
			shm_ptr[0].nanosec = 0;
		}
	}

	printf("Master: Properly terminating\n");
	fprintf(file_ptr, "Master: Properly terminating\n");

	// Writing stats
	printf("Statistics for this run:\n");
	printf("- Total deadlock occurences = %d\n", total_deadlock);
	printf("- Total processes spawned = %d\n", total_processes);
	printf("- Total processes reaped = %d\n", total_reaped);
	printf("- Total unsafe states detected = %d\n", total_blocked);
	printf("- Average run time for all processes is %d:%d\n", avg_run_sec/total_reaped, avg_run_nanosec/total_reaped);
	printf("- Average wait time for all processes is %d:%d\n", avg_wait_sec/total_waits, avg_wait_nanosec/total_waits);

	fprintf(file_ptr, "Statistics for this run:\n");
	fprintf(file_ptr, "- Total deadlock occurences = %d\n", total_deadlock);
	fprintf(file_ptr, "- Total processes spawned = %d\n", total_processes);
	fprintf(file_ptr, "- Total processes reaped = %d\n", total_reaped);
	fprintf(file_ptr, "- Total unsafe states detected = %d\n", total_blocked);
	fprintf(file_ptr, "- Average run time for all processes is %d:%d\n", avg_run_sec/total_reaped, avg_run_nanosec/total_reaped);
	fprintf(file_ptr, "- Average wait time for all processes is %d:%d\n", avg_wait_sec/total_waits, avg_wait_nanosec/total_waits);


	// Reaping leftover processes in wait and active queue; Also empty the queues
	queue_size = wait_queue->size;
	for (i = 0; i < queue_size; i++) {
		r_node = derqueue(wait_queue);
		local_pid = r_node->request.local_pid;
		m_queue.mtype = local_pid +1;
		m_queue.msg.local_pid = -100; // Sending a -100 local pid tell the child to end
		// Send message
		if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0) == -1){
			perror("Master: processor(): msgsnd()");
			cleanup();
			return -1;
		}
		wait(&stat);
	}
	queue_size = active_queue->size;
	for (i = 0; i < queue_size; i++) {
		r_node = derqueue(active_queue);
		local_pid = r_node->request.local_pid;
		m_queue.mtype = local_pid +1;
		m_queue.msg.local_pid = -100; // Sending a -100 local pid tell the child to end
		// Send message
		if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0) == -1){
			perror("Master: processor(): msgsnd()");
			cleanup();
			return -1;
		}
		wait(&stat);
	}

	// Detach/closes any opening resources
	cleanup();
	return 0;
}

// Kill all children
static void sig_handler(int s){
	printf("Master: OSS could not finish its run. Most likely due interruption or non-stop deadlocking.\n");
	int i = 0;
	switch(s){
		// Handle alarm from real timer
		case SIGALRM:
			printf("%d seconds of real time has passed. Terminating...\n", real_time);
			for( i = 0; i < CHILDREN; i++){
				if (p_tracker[i] == 1)
					if(kill(child_pid[i], SIGKILL) == 0){
						printf("Master: %d killed from SIGALARM\n", child_pid[i]);
					}
			}
			break;
		// Handle anyother interuptions
		case SIGINT:
			for( i = 0; i < CHILDREN; i++){
				if (p_tracker[i] == 1)
					if(kill(child_pid[i], SIGKILL) == 0){
						printf("Master: %d killed from SIGINT\n", child_pid[i]);
					}
			}
			break;
	}
	// No matter the signal always cleanup
	cleanup();
	exit(0);
}

// Cleanup function
void cleanup(){
	if (wait_queue != NULL) {
		free(wait_queue);
	}
	if (active_queue != NULL) {
		free(active_queue);
	}
	if(shmdt(shm_ptr) == -1){
		perror("Master: shmdt");
	}
	if (shmctl(shm_id, IPC_RMID, NULL) == -1){
		perror("Master: shmctl");
	}
	msgctl(msg_snd_id, IPC_RMID, NULL);
	msgctl(msg_rcv_id, IPC_RMID, NULL);
	if (file_ptr != NULL){
		if (fclose(file_ptr) == EOF){
			perror("Master: fclose()");
		}
	}
}
