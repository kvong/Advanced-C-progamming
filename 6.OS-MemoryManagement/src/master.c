#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <shmry.h>
#include <request.h>
#include <msgqueue.h>
#include <spawnclock.h>
#include <pqueue.h>
#include <queue.h>
#include <node.h>

/******************************************************************************
* File:             master.c
*
* Author:           Khanh Vong  
* Created:          12/03/19 
*                   OS simulator for memory management using Paging with First-
*                   In First-Out and Least-Recently-Used replacement policy.
*****************************************************************************/


// OSS Constant Controls
#define CHILDREN 18
#define PBOUND 100

#define MAINMEMORY 256	// Main Memory Frames
#define PAGETABLES 576	// 32 max memory for all 18 process: 32*18 = 576
#define KSIZE 1024		// B in KB

// Process Control block
#define PCBSIZE 3
#define OCCUPANCY 0
#define LBOUND 1
#define UBOUND 2

// Page/Frame data block
#define PGBSIZE 4
#define LOCALPID 0
#define FRAMENUM 1
#define PAGENUM 1
#define REFBYTE 2
#define DIRTYBIT 3


// Default conditions for oss
const int real_time = 2;

FILE* file_ptr = NULL;
pid_t child_pid[CHILDREN];
int shm_id, msg_snd_id, msg_rcv_id;
static shmry_t* shm_ptr;
static msgqueue_t m_queue;
pqueue_t* wait_queue = NULL;

// Control structures
int p_tracker[CHILDREN][PCBSIZE];

// Prototypes
void clean_file(char*);
int processor(char*, int);
static void sig_handler(int);
void cleanup();

int lg(double);
int update_refbyte(int);

int has_opening(int [CHILDREN][PCBSIZE], int);
int available_frames(int [MAINMEMORY][4]);
int opening_frames(int [MAINMEMORY][4]);
int is_occupied(int [MAINMEMORY][4], int);
int page_fault(int [PAGETABLES][4], int, int);

int main(int argc, char *argv[])
{	
	int arg = argc;
	int opt;

	while((opt = getopt(argc, argv, ":flh")) != -1){
		switch (opt) {
			case 'f':
				printf("Performing Memory Management with FIFO replacement policy.\n");
				arg = 1;
				break;
			case 'l':
				printf("Performing Memory Management with LRU replacement policy\n");
				arg = 0;
				break;
			case 'h':
				help();
				return 0;
				break;
			case '?':
				printf("Unknown option: %s\n", optopt);
				help();
				return 1;
				break;
			default:
				help();
				return 1;
				break;
		}
	}
	
	//Set up master alarm
	signal(SIGALRM, (void (*)(int))sig_handler);
	signal(SIGINT, (void (*)(int))sig_handler);
	alarm(real_time);

	if (processor("oss.log", arg) == -1) {
		printf("OSS: Failure while processing.\n");
	}
	return 0;
}

// Print help message
void help(){
	printf("USAGE: oss -[f|l]\n");
	printf("f: First In First Out\n");
	printf("l: Least Recently Used\n");
}

// Binary log
int lg(double n){
	return log(n)/log(2);
}

// Clean file for writing
void clean_file(char* input){
	file_ptr = fopen(input, "w+");
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

// Check openings to launch process
int has_opening(int block[CHILDREN][3], int size){
	int i;
	for (i = 0; i < size; i++) {
		if (block[i][OCCUPANCY] == -1) {
			return i;
		}
	}
	return -1;
}

// Look for opening frames
int available_frames(int block[MAINMEMORY][PGBSIZE]){
	int i;
	for (i = 0; i < MAINMEMORY; i++) {
		if (block[i][LOCALPID] == -1) {
			return i;
		}
	}
	return -1;
}

int opening_frames(int block[MAINMEMORY][4]){
	int i, openings = 0;
	for (i = 0; i < MAINMEMORY; i++) {
		if (block[i][0] == -1) {
			return i;
		}
	}
	return -1;
}

// Check PCB for occupancy
int is_occupied(int block[MAINMEMORY][4], int local_pid){
	int i, occupied = 0;
	for (i = 0; i < MAINMEMORY; i++) {
		if (local_pid == block[i][LOCALPID]) {
			occupied++;
		}
	}
	return occupied;
}

// Check pagefault
int page_fault(int p_table[PAGETABLES][PGBSIZE], int local_pid, int request){
	int page_index = local_pid*32 + request;
	if (p_table[page_index][FRAMENUM] >= 0) {
		return -1;
	}
	return 1;
}

// Add new most significant bit 
int update_refbyte(int current){
	int new_refbyte, current_degree;
	if (current == 0) {
		return 1;
	}
	else{
		current_degree = lg((double)current);
		new_refbyte = current | (int)(pow(2, current_degree + 1));
	}
	if (new_refbyte >= 256) {
		new_refbyte = 256;
	}
	return new_refbyte;
}

// Perform simulation
int processor(char* filename, int arg){
	int nxt_spawn_nan, nxt_spawn_sec;
	int stat;
	int local_pid, framenum, refbyte, dirtybit;
	int page_number;
	int request_addr;
	int total_processes = 0;
	int total_terminated = 0;
	int opening;
	int i, j, queue_size;
	int mem_requirement;
	int operation;
	queue_t* fifo_selection = init_queue();
	node_t* fifo_node;
	int lru_frame[PGBSIZE];
	int grant_sec, grant_nanosec;
	int total_access = 0;
	int access_sec = 0;
	int access_nanosec = 0;
	int request_sec = 0;
	int request_nanosec = 0;
	int dot_counter = 0;

	int fifo_fault = 0;
	int fifo_hit = 0;
	int lru_fault = 0;
	int lru_hit = 0;
	int fault = 0;
	int hit = 0;
	int total_grant;
	int total_request = 0;

	int frame_index;
	request_t tmp;
	pnode_t* node = NULL;

	int frame_table[MAINMEMORY][PGBSIZE];
	int page_tables[PAGETABLES][PGBSIZE];

	wait_queue = init_pqueue();

	srand(time(NULL)*getpid());
	clean_file(filename);

	// String argument for child
	char exe_name[20];
	char exe_arg_1[10];
	char exe_arg_2[10];
	strcpy(exe_name, "");
	strcpy(exe_name, "./dostuff");

	// Open file for appending
	file_ptr = fopen(filename, "a");
	if(!file_ptr){
		perror("oss: Error: 'clean_file()' failed at fopen()");
		exit(EXIT_FAILURE);
	}

	// Initialize shared memory
	if (init_shmry(6666) == -1){
		cleanup();
		exit(EXIT_FAILURE);
	}

	// Initialize a named semaphore
	if ((shm_ptr[0].mutex = sem_open(SNAME, O_CREAT, PERMS, 1)) == SEM_FAILED){
		perror("oss: Error: sem_open");
		exit(EXIT_FAILURE);
	}
	// Close because we wont use it in parent
	if ( sem_close(shm_ptr[0].mutex) == -1){
		perror("oss: Error: sem_close");
		sem_unlink(SNAME);
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

	nxt_spawn_nan = get_randnan();
	nxt_spawn_sec = get_randsec();
	shm_ptr[0].sec = nxt_spawn_sec;
	shm_ptr[0].nanosec = nxt_spawn_nan;
	nxt_spawn_nan = shm_ptr[0].nanosec + get_randnan();
	nxt_spawn_sec = shm_ptr[0].sec + get_randsec();
	if ( nxt_spawn_nan/SECOND == 1){
		nxt_spawn_sec++;
		nxt_spawn_nan = nxt_spawn_nan % SECOND;
	}

	// Initialize page/frame table
	for (i = 0; i < PAGETABLES; i++) {
		if (i < MAINMEMORY) {
			frame_table[i][LOCALPID] = -1;
			frame_table[i][PAGENUM] = -1;
			frame_table[i][REFBYTE] = 0;
			frame_table[i][DIRTYBIT] = 0;
			
		}
		page_tables[i][LOCALPID] = -1;
		page_tables[i][FRAMENUM] = -1;
		page_tables[i][REFBYTE] = -1;
		page_tables[i][DIRTYBIT] = -1;

	}

	// Init PCB
	for (i = 0; i < CHILDREN; i++) {
		p_tracker[i][OCCUPANCY] = -1;
	}

	local_pid = 0;
	tmp.local_pid = local_pid;
	tmp.page_number = 0;
	total_processes++;

	// Get and set max memory requirement
	mem_requirement = rand() % 23 + 10;
	p_tracker[local_pid][OCCUPANCY] = 1;
	p_tracker[local_pid][LBOUND] = local_pid*32;
	p_tracker[local_pid][UBOUND] = (local_pid*32) + mem_requirement;

	fprintf(file_ptr, "Master: Child %d launched with %dKb requirement at time%d:%d\n", local_pid, mem_requirement, shm_ptr[0].sec, shm_ptr[0].nanosec);
	child_pid[local_pid] = fork();
	if (child_pid[local_pid] == 0){
		// Add an additional commandline argument to keep track of which child has exitted
		sprintf(exe_arg_1, "%d", local_pid);
		sprintf(exe_arg_2, "%d", mem_requirement);
		char *args[] = { exe_name, exe_arg_1, exe_arg_2, NULL};
		execlp(args[0], args[1], args[2], args);
	}

	while(total_terminated < PBOUND){
		// If there is a message in queue
		if (msgrcv(msg_rcv_id, &m_queue, sizeof(m_queue.msg), 0, IPC_NOWAIT) >= 0){
			local_pid = m_queue.msg.local_pid;
			page_number = m_queue.msg.page_number;
			request_addr = m_queue.msg.request_addr;
			operation = m_queue.msg.operation;
			request_sec = m_queue.msg.request_sec;
			request_nanosec = m_queue.msg.request_nanosec;
			total_request++;
			
			// Check message -1 means termination
			if (page_number >= 0) {
				fprintf(file_ptr, "Master: Child %d request for address %d on page %d at time %d:%d\n", local_pid, request_addr, page_number, shm_ptr[0].sec, shm_ptr[0].nanosec);
				if (page_fault(page_tables, local_pid, page_number) == 1) {
					// Page fault
					fprintf(file_ptr, "Master: Child %d PAGE FAULT on request for pg%d\n", local_pid, page_number);
					fault++;

					tmp.local_pid = local_pid;
					tmp.page_number = page_number;
					tmp.request_addr = request_addr;
					tmp.operation = operation;
					tmp.request_sec = request_sec;
					tmp.request_nanosec = request_nanosec;

					tmp.grant_sec = shm_ptr[0].sec + get_randsec();
					tmp.grant_nanosec = shm_ptr[0].nanosec + get_randnan();
					if ( tmp.grant_nanosec/SECOND == 1){
						tmp.grant_sec++;
						tmp.grant_nanosec = tmp.grant_nanosec % SECOND;
					}
					enpqueue(wait_queue, tmp);
				}
				else{ 
					// No page fault
					fprintf(file_ptr, "Master: Child %d PAGE HIT on request for pg%d\n", local_pid, page_number);
					hit++;

					if (msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0) == -1){
						perror("Master: processor(): msgsnd()");
						cleanup();
						return -1;
					}
					shm_ptr[0].nanosec += 10;	
					if ( shm_ptr[0].nanosec/SECOND == 1){
						shm_ptr[0].sec++;
						shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
					}

					access_nanosec += 10;

					refbyte = frame_table[page_tables[local_pid*32 + page_number][FRAMENUM]][REFBYTE];
					refbyte = update_refbyte(refbyte);
					frame_table[page_tables[local_pid*32 + page_number][FRAMENUM]][REFBYTE] = refbyte;
					if (operation == 1) {
						frame_table[page_tables[local_pid*32 + page_number][FRAMENUM]][DIRTYBIT] = 1;
						page_tables[local_pid*32 + page_number][DIRTYBIT] = 1;
						fprintf(file_ptr, "Master: Grant P%d request to WRITE to frame %d containing page %d. Dirty bit set.\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[page_tables[local_pid*32 + page_number][FRAMENUM]][PAGENUM]);
					}
					else{ 
						frame_table[page_tables[local_pid*32 + page_number][FRAMENUM]][DIRTYBIT] = 0;
						page_tables[local_pid*32 + page_number][DIRTYBIT] = 0;
						fprintf(file_ptr, "Master: Grant P%d request to READ to frame %d containing page %d\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[page_tables[local_pid*32 + page_number][FRAMENUM]][PAGENUM]);
					}
				}
			}
			else{
				// Reap terminated process
				wait(&stat);
				local_pid = WEXITSTATUS(stat);
				fprintf(file_ptr, "Master: Child %d terminated at %d:%d\n", local_pid, shm_ptr[0].sec, shm_ptr[0].nanosec);
				total_terminated++;

				queue_size = fifo_selection->size;
				for (i = 0; i < queue_size; i++) {
					if (fifo_selection->front->local_pid == local_pid) {
						dequeue(fifo_selection);
					}
					else{
						enqueue(fifo_selection, fifo_selection->front->local_pid, fifo_selection->front->frame_number);
						dequeue(fifo_selection);
					}
				}
				
				for (i = p_tracker[local_pid][LBOUND]; i < p_tracker[local_pid][UBOUND]; i++) {
					page_tables[i][LOCALPID] = -1;
					page_tables[i][FRAMENUM] = -1;
					page_tables[i][REFBYTE] = 0;
					page_tables[i][DIRTYBIT] = 0;
				}

				p_tracker[local_pid][OCCUPANCY] = -1;
				p_tracker[local_pid][LBOUND] = 0;
				p_tracker[local_pid][UBOUND] = 0;

				for (i = 0; i < MAINMEMORY; i++) {
					if (frame_table[i][LOCALPID] == local_pid) {
						frame_table[i][LOCALPID] = -1;
						frame_table[i][PAGENUM] = -1;
						frame_table[i][REFBYTE] = 0;
						frame_table[i][DIRTYBIT] = 0;
					}
				}

				fprintf(file_ptr, "Master: Terminated total is %d\n", total_terminated);
				fprintf(file_ptr, "*Frame Table:");
				for (i = 0; i < MAINMEMORY; i++) {
					if (i % 32 == 0) {
						fprintf(file_ptr, "\n");
					}
					if (frame_table[i][LOCALPID] != -1) {
						fprintf(file_ptr, "+");
					}
					else{
						fprintf(file_ptr, ".");
					}
				}
				fprintf(file_ptr, "\n");
			}
		}

		// Determine to grant request in wait queue
		if (wait_queue->size > 0) {
			if ((wait_queue->front->request.grant_sec < shm_ptr[0].sec) || 
					(wait_queue->front->request.grant_sec == shm_ptr[0].sec && 
					 wait_queue->front->request.grant_nanosec <= shm_ptr[0].nanosec)) {
				fprintf(file_ptr, "Master: Process %d request for page %d grantable at %d:%d\n", wait_queue->front->request.local_pid, wait_queue->front->request.page_number, shm_ptr[0].sec, shm_ptr[0].nanosec);

				local_pid = wait_queue->front->request.local_pid;
				page_number = wait_queue->front->request.page_number;
				request_addr = wait_queue->front->request.request_addr;
				operation = wait_queue->front->request.operation;
				grant_sec = wait_queue->front->request.grant_sec;
				grant_nanosec = wait_queue->front->request.grant_nanosec;
				request_sec = wait_queue->front->request.request_sec;
				request_nanosec = wait_queue->front->request.request_nanosec;

				// Look for opening in frame table
				if ((opening = opening_frames(frame_table)) != -1) {
					fprintf(file_ptr, "Master: Opening frame detected\n");
					m_queue.mtype = local_pid + 1;
					m_queue.msg.local_pid = local_pid;
					m_queue.msg.page_number = page_number;
					m_queue.msg.request_addr = request_addr;
					m_queue.msg.grant_sec = grant_sec;
					m_queue.msg.grant_nanosec = grant_nanosec;

					access_sec += shm_ptr[0].sec - request_sec;
					access_nanosec += abs(shm_ptr[0].nanosec - request_nanosec);

					page_tables[local_pid*32 + page_number][LOCALPID] = local_pid;
					page_tables[local_pid*32 + page_number][FRAMENUM] = opening;
					page_tables[local_pid*32 + page_number][DIRTYBIT] = 1;

					fprintf(file_ptr, "Master: Process %d page table updated\n", local_pid);
					frame_table[opening][LOCALPID] = local_pid;
					frame_table[opening][PAGENUM] = page_number;
					frame_table[opening][REFBYTE] = 1;

					frame_table[opening][DIRTYBIT] = operation & 1;

					fprintf(file_ptr, "Master: Page %d of process %d has been loaded into frame %d\n", m_queue.msg.page_number, m_queue.msg.local_pid, opening);

					// Processing dirty bit
					if (operation == 1) {
						frame_table[opening][DIRTYBIT] = 1;
						page_tables[local_pid*32 + page_number][DIRTYBIT] = 1;
						fprintf(file_ptr, "Master: Grant P%d request to WRITE to frame %d containing page %d. Dirty bit set.\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[opening][PAGENUM]);
					}
					else{ 
						frame_table[opening][DIRTYBIT] = 0;
						page_tables[local_pid*32 + page_number][DIRTYBIT] = 0;
						fprintf(file_ptr, "Master: Grant P%d request to READ to frame %d page %d.\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[opening][PAGENUM]);
					}


					fprintf(file_ptr, "Master: Process %d request for page %d has been granted \n", m_queue.msg.local_pid, m_queue.msg.page_number);

					msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0);
					depqueue(wait_queue);
					enqueue(fifo_selection, m_queue.msg.local_pid, opening);
				}
				else{
					// Perform replacement policy
					fprintf(file_ptr, "Master: Frames are full performing replacement policy\n");

					if (arg == 1) {
						// FIFO replacement policy
						fifo_node = dequeue(fifo_selection);
						fprintf(file_ptr, "FIFO: Replacing frame %d\n", fifo_node->frame_number);

						page_tables[frame_table[fifo_node->frame_number][LOCALPID]*32 + frame_table[fifo_node->frame_number][PAGENUM]][FRAMENUM] = -1;
						page_tables[frame_table[fifo_node->frame_number][LOCALPID]*32 + frame_table[fifo_node->frame_number][PAGENUM]][DIRTYBIT] = 0;

						m_queue.mtype = local_pid + 1;
						m_queue.msg.local_pid = local_pid;
						m_queue.msg.page_number = page_number;
						m_queue.msg.request_addr = request_addr;
						m_queue.msg.grant_sec = grant_sec;
						m_queue.msg.grant_nanosec = grant_nanosec;

						access_sec += shm_ptr[0].sec - request_sec;
						access_nanosec += abs(shm_ptr[0].nanosec - request_nanosec);

						shm_ptr[0].sec = grant_sec;
						shm_ptr[0].nanosec = grant_nanosec;

						page_tables[local_pid*32 + page_number][FRAMENUM] = fifo_node->frame_number;
						page_tables[local_pid*32 + page_number][DIRTYBIT] = operation & 1;

						frame_table[fifo_node->frame_number][LOCALPID] = local_pid;
						frame_table[fifo_node->frame_number][PAGENUM] = page_number;
						frame_table[fifo_node->frame_number][REFBYTE] = 1;
						frame_table[fifo_node->frame_number][DIRTYBIT] = operation & 1;

						if (operation == 1) {
							frame_table[fifo_node->frame_number][DIRTYBIT] = 1;
							page_tables[local_pid*32 + page_number][DIRTYBIT] = 1;
							fprintf(file_ptr, "Master: Grant P%d request to WRITE to frame %d containing page %d. Dirty bit set.\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[fifo_node->frame_number][PAGENUM]);
						}
						else{ 
							frame_table[opening][DIRTYBIT] = 0;
							page_tables[local_pid*32 + page_number][DIRTYBIT] = 0;
							fprintf(file_ptr, "Master: Grant P%d request to READ to frame %d containing page %d\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[fifo_node->frame_number][PAGENUM]);
						}

						fprintf(file_ptr, "Master: Page %d of process %d has been loaded into frame %d\n", page_number, local_pid, fifo_node->frame_number);


						fprintf(file_ptr, "Master: Process %d request for page %d has been granted\n", local_pid, page_number);

						msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0);
						depqueue(wait_queue);
						enqueue(fifo_selection, local_pid, fifo_node->frame_number);
					}
					else{
						// Find Least Recently Used frame
						lru_frame[REFBYTE] = frame_table[0][REFBYTE];
						frame_index = 0;
						for (i = 0; i < MAINMEMORY; i++) {
							if (frame_table[i][REFBYTE] < lru_frame[REFBYTE]) {
								lru_frame[LOCALPID] = frame_table[i][LOCALPID];
								lru_frame[PAGENUM] = frame_table[i][PAGENUM];
								lru_frame[REFBYTE] = frame_table[i][REFBYTE];
								lru_frame[DIRTYBIT] = frame_table[i][DIRTYBIT];
								frame_index = i;
							}
						}
						fprintf(file_ptr, "LRU: Replacing frame %d\n", frame_index);

						page_tables[lru_frame[LOCALPID]*32 + lru_frame[PAGENUM]][FRAMENUM] = -1;
						page_tables[lru_frame[LOCALPID]*32 + lru_frame[PAGENUM]][DIRTYBIT] = 0;

						m_queue.mtype = local_pid + 1;
						m_queue.msg.local_pid = local_pid;
						m_queue.msg.page_number = page_number;
						m_queue.msg.request_addr = request_addr;
						m_queue.msg.grant_sec = grant_sec;
						m_queue.msg.grant_nanosec = grant_nanosec;

						access_sec += shm_ptr[0].sec - request_sec;
						access_nanosec += abs(shm_ptr[0].nanosec - request_nanosec);

						shm_ptr[0].sec = grant_sec;
						shm_ptr[0].nanosec = grant_nanosec;
						
						page_tables[local_pid*32 + page_number][FRAMENUM] = frame_index;
						page_tables[local_pid*32 + page_number][DIRTYBIT] = operation & 1;

						
						frame_table[frame_index][LOCALPID] = local_pid;
						frame_table[frame_index][PAGENUM] = page_number;
						frame_table[frame_index][REFBYTE] = 1;
						frame_table[frame_index][DIRTYBIT] = operation & 1;

						if (operation == 1) {
							frame_table[frame_index][DIRTYBIT] = 1;
							page_tables[local_pid*32 + page_number][DIRTYBIT] = 1;
							fprintf(file_ptr, "Master: Grant P%d request to WRITE to frame %d containing page %d. Dirty bit set.\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[frame_index][PAGENUM]);
						}
						else{ 
							frame_table[frame_index][DIRTYBIT] = 0;
							page_tables[local_pid*32 + page_number][DIRTYBIT] = 0;
							fprintf(file_ptr, "Master: Grant P%d request to READ to frame %d containing page %d\n", local_pid, page_tables[local_pid*32 + page_number][FRAMENUM], frame_table[frame_index][PAGENUM]);
						}

						fprintf(file_ptr, "Master: Page %d of process %d has been loaded into frame %d\n", page_number, local_pid, frame_index);

						fprintf(file_ptr, "Master: Process %d request for page %d has been granted\n", local_pid, page_number);

						msgsnd(msg_snd_id, &m_queue, sizeof(m_queue.msg), 0);
						depqueue(wait_queue);
					}
				}
			}
		}

		// Spawn process at random
		if (total_processes < PBOUND) {
			if (nxt_spawn_sec <= shm_ptr[0].sec){
				local_pid = has_opening(p_tracker, CHILDREN);
				if ((nxt_spawn_nan <= shm_ptr[0].nanosec && local_pid >= 0) || (nxt_spawn_sec < shm_ptr[0].sec && local_pid >= 0)) {
					total_processes++;
					// Get next spawn time
					nxt_spawn_sec = shm_ptr[0].sec + get_randsec();
					nxt_spawn_nan = shm_ptr[0].nanosec + get_randnan();
					if (nxt_spawn_nan/SECOND == 1) {
						nxt_spawn_sec++;
						nxt_spawn_nan = nxt_spawn_nan % SECOND;
					}
					tmp.local_pid = local_pid;
					tmp.page_number = 0;

					mem_requirement = rand() % 23 + 10;

					p_tracker[local_pid][OCCUPANCY] = 1;
					p_tracker[local_pid][LBOUND] = local_pid*32;
					p_tracker[local_pid][UBOUND] = (local_pid*32) + mem_requirement;
					// Create page table for this process

					fprintf(file_ptr, "Master: Child %d launched with %dKb requirement\n", local_pid, mem_requirement);
					child_pid[local_pid] = fork();
					if (child_pid[local_pid] == 0) {
						// Create string to exec
						sprintf(exe_arg_1, "%d", local_pid);
						sprintf(exe_arg_2, "%d", mem_requirement);
						char *args[] = { exe_name, exe_arg_1, exe_arg_2, NULL};
						execlp(args[0], args[1], args[2], args);
					}
				}
			}
		}

		// Increment clock
		shm_ptr[0].nanosec += 40;	
		if ( shm_ptr[0].nanosec/SECOND == 1){
			shm_ptr[0].sec++;
			shm_ptr[0].nanosec = shm_ptr[0].nanosec % SECOND;
			fprintf(file_ptr, "Master: Current time %d:%d\n", shm_ptr[0].sec, shm_ptr[0].nanosec);
			// Print page and frame table every second of logical time
			fprintf(file_ptr, "*Frame Table:");
			for (i = 0; i < MAINMEMORY; i++) {
				if (i % 32 == 0) {
					fprintf(file_ptr, "\n");
				}
				if (frame_table[i][LOCALPID] != -1) {
					fprintf(file_ptr, "+");
				}
				else{
					fprintf(file_ptr, ".");
				}
			}
			fprintf(file_ptr, "\n");
			fprintf(file_ptr, "*Corresponding Page Table:");
			for (i = 0; i < PAGETABLES; i++) {
				if (i % 32 == 0) {
					fprintf(file_ptr, "\n");
				}
				if (page_tables[i][LOCALPID] != -1) {
					fprintf(file_ptr, "+");
				}
				else{
					fprintf(file_ptr, ".");
				}
			}
			fprintf(file_ptr, "\n");
			fprintf(file_ptr, "*Max allocation page table\n");
			for (i = 0; i < CHILDREN; i++) {
				for (j = 0; j < 32; j++) {
					if ( j < p_tracker[i][UBOUND] - p_tracker[i][LBOUND]) {
						fprintf(file_ptr, "+");
					}
					else{
						fprintf(file_ptr, ".");
					}
				}
				fprintf(file_ptr, "\n");
			}
		}

		total_grant = fault + hit;
		if (total_grant % 200 == 0 && total_grant != 0) {
			fprintf(file_ptr, "Master: Right shift in Reference Byte\n");
			for (i = 0; i < MAINMEMORY; i++) {
				frame_table[i][REFBYTE] = frame_table[i][REFBYTE]>>1;
			}
		}
	}

	// Printing stats
	printf("Stats:\n");
	if (arg == 1) {
		printf("FIFO: Total Faults = %d\tTotal Hits = %d\n", fault, hit);
	}
	else
		printf("LRU: Total Faults = %d\tTotal Hits = %d\n", fault, hit);

	total_access = fault + hit;
	printf("Total logical run time = %d:%d\n", shm_ptr[0].sec, shm_ptr[0].nanosec );
	printf("Number of memory accessed per second %f\n", (float)total_access/shm_ptr[0].sec);
	printf("Number of PAGE FAULTS per access %f\n", (float)total_access/fault);
	printf("Number of PAGE HITS per access %f\n", (float)total_access/hit);
	printf("Average access speed %d:%d\n", access_sec/total_access, access_nanosec/total_access);
	fclose(file_ptr);
	file_ptr = NULL;
	cleanup();
	return 1;
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
				if (p_tracker[i][OCCUPANCY] == 1)
					if(kill(child_pid[i], SIGKILL) == 0){
						printf("Master: %d killed from SIGALARM\n", child_pid[i]);
					}
			}
			break;
		// Handle anyother interuptions
		case SIGINT:
			for( i = 0; i < CHILDREN; i++){
				if (p_tracker[i][OCCUPANCY] == 1)
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
	/*
	if (wait_queue != NULL) {
		free(wait_queue);
	}
	*/
	// Unlink semaphore
	if (sem_unlink(SNAME) == -1){
		perror("oss: Error: sem_unlink");
		cleanup();
		exit(EXIT_FAILURE);
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
