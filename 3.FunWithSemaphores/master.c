#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "shmry.h"

// Author: Khanh Vong
// Date: 10/4/2019
// Description: Operating system simulator that manages s processes

// Loose ends to take care of at termination
FILE* file_ptr = NULL;
pid_t* child_pid;
static shmry_t* shm_ptr;
int shm_id;

// Default conditions for oss
int real_time = 5;
int sim_time = 2;
int children = 5;
int p_limit = 100;
int processed = 0;
int nanosec = 0;
int sec = 0;

// Prototypes
void clean_file(char*);
int processor(char*);
static void sig_handler(int);
void cleanup();

int main(int argc, char** argv){
	//Set up master alarm
	signal(SIGALRM, (void (*)(int))sig_handler);
	signal(SIGINT, (void (*)(int))sig_handler);
	alarm(real_time);

	// Defaut outputfile will be called oss.log
	char output[20] = "oss.log";
	int option;
	while((option = getopt(argc, argv, ":hs:l:t:p:")) != -1){
		switch(option){
			case 'h':
				printf("Usage: oss [[-s child_processes][-l filename][-t time][-p process_cap]] [-h].\n");
				return 0;
			case 's':
				children = atoi(optarg);
				printf("Number of active children: %d\n", children);
				if (children > p_limit){
					printf("Max process allowance is %d.", p_limit);
					return 0;
				}
				break;
			case 'l':
				strcpy(output, "");
				strcpy(output, optarg);
				printf("Output = '%s'\n", output);
				break;
			case 't':
				real_time = atoi(optarg);
				printf("Real kill time = %d\n", real_time);
				break;
			case 'p':
				p_limit = atoi(optarg);
				printf("Process limit = %d.\n", p_limit);
				break;
			case ':':
				printf("No value specified. Terminating...\n");
				return 1;
			case '?':
				printf("Unknown option: %s.\n", optopt);
				return 1;
		}		
	}

	// Create an array of children
	child_pid = malloc(children * sizeof(pid_t));

	// Manage processes
	processor(output);
	perror("Program exit");
	free(child_pid);
	return 0;
}

void clean_file(char* input){
	file_ptr = fopen(input, "w");
	if(!file_ptr){
		printf("File '%d' could not be opened for cleaning.\n", input);
		perror("oss: Error: 'clean_file()' failed at fopen()");
		exit(EXIT_FAILURE);
	}
	fprintf(file_ptr, "");
	fclose(file_ptr);
}

// Simulate os
int processor(char* output){
	// Clean file for writing
	clean_file(output);

	// Open file for appending
	file_ptr = fopen(output, "a");
	if(!file_ptr){
		perror("oss: Error: 'clean_file()' failed at fopen()");
		exit(EXIT_FAILURE);
	}
	
	// Shared memory section
	key_t key = ftok("./", 100);
	shm_id = shmget(key, sizeof(shmry_t), IPC_CREAT | PERMS);
	if (shm_id == -1){
		perror("oss: Error: shm_id");
		return 1;
	}
	shm_ptr = (shmry_t *)shmat(shm_id, NULL, 0);
	if ( shm_ptr == -1 ){
		perror("oss: Error: shmat");
		return 1;
	}

	// Set all time to zero and indicate that message box is empty
	shm_ptr[0].sec = 0;
	shm_ptr[0].nanosec = 0;
	shm_ptr[0].msg.sec = 0;
	shm_ptr[0].msg.nanosec = 0;
	shm_ptr[0].msg.empty = 1;
	
	// Initialize a named semaphore
	if ((shm_ptr[0].mutex = sem_open(SNAME, O_CREAT, PERMS, 1)) == SEM_FAILED){
		perror("oss: Error: sem_open");
		exit(EXIT_FAILURE);
	}
	// Close because we wont use it in parent
	if ( sem_close(shm_ptr[0].mutex) == -1){
		perror("oss: Error: sem_close");
		sem_unlink(SNAME);
		exit("EXIT_FAILURE");
	}
	
	/* Uncomment to setup signal in processor
	//Set up master alarm
	signal(SIGALRM, (void (*)(int))sig_handler);
	signal(SIGINT, (void (*)(int))sig_handler);
	alarm(real_time);
	*/

	// Forking
	int active_process = 0;
	int i;
	int stat;
	int total_processes = 0;
	pid_t pid;
	
	// String argument for child
	char exe_name[20];
	char exe_arg[10];
	strcpy(exe_name, "");
	strcpy(exe_name, "./dostuff");

	// Fork off n processes
	for ( active_process; active_process < children; active_process++){
		total_processes++;

		// Child execute dostuff
		child_pid[active_process] = fork();
		if (child_pid[active_process] == 0){
			// Create string to exec
			printf("Master: child_pid %d created\n", getpid());
			// Add an additional commandline argument to keep track of which child has exitted
			sprintf(exe_arg, "%d", active_process);
			char *args[] = { exe_name, exe_arg, NULL};
			execlp(args[0], args[1], args);
		}
	}

	// Ensure there are s processes at all; time will increment while wait
	while((pid = waitpid(0, &stat, WNOHANG)) == 0 || total_processes <= p_limit){
		// Make sure to only launch process when shmg is empty
		if (shm_ptr[0].msg.empty == 0 && WIFEXITED(stat) && pid > 0){
			stat = WEXITSTATUS(stat);
			active_process--;
			printf("Master: Child #%d with pid %d has sent a message.\n", stat, child_pid[stat]);
			// Write to file
			fprintf(file_ptr, "Master: Child pid %d is terminating at my time %d.%d ", child_pid[stat], shm_ptr[0].sec, shm_ptr[0].nanosec);
			fprintf(file_ptr, "because it reached %d.%d in child process.\n", shm_ptr[0].msg.sec, shm_ptr[0].msg.nanosec);
			// Empty message box
			shm_ptr[0].msg.empty = 1;
			active_process++;
			total_processes++;
			if (total_processes > p_limit){
				printf("Master: Maximum created. Tying up loose ends and terminating...\n");
				sig_handler(SIGINT);
				break;
			}

			// Launch new child
			child_pid[stat] = fork();
			if (child_pid[stat] == 0){
				printf("Master: child #%d new pid is %d\n", stat, getpid());
				// Create string to exec
				sprintf(exe_arg, "%d", stat);
				char *args[] = { exe_name, exe_arg, NULL};
				execlp(args[0], args[1], args);
			}
		}
		shm_ptr[0].nanosec += NANOSEC;	
		if ( shm_ptr[0].nanosec/SECOND == 1){
			shm_ptr[0].sec++;
			shm_ptr[0].nanosec = 0;
		}
		if (shm_ptr[0].sec == 2){
			printf("2 simulated seconds reached.\n");
			sig_handler(SIGINT);
			break;
		}
	}

	// All processes are done, tie up loose ends
	for( i = 0; i < children; i++){
		// If kill return an error
		if(kill(child_pid[i], SIGTERM) == -1){
			perror("oss: processor(): kill()");
			cleanup();
			exit(EXIT_FAILURE);
		}
		else
			printf("%d killed through SIGTERM\n", child_pid[i]);
	}

	// Unlink semaphore
	if (sem_unlink(SNAME) == -1){
		perror("oss: Error: sem_unlink");
		cleanup();
		exit(EXIT_FAILURE);
	}
	// Detach/closes any opening resources
	cleanup();
	return 0;
}

// Kill all children
static void sig_handler(int s){
	int i = 0;
	switch(s){
		// Handle alarm from real timer
		case SIGALRM:
			printf("%d seconds of real time has passed. Terminating...\n", real_time);
			for( i = 0; i < children; i++){
				if(kill(child_pid[i], SIGKILL) == 0){
					while(waitpid(-1, &stat, WNOHANG)== 0);
					printf("Master: %d killed from SIGALARM\n", child_pid[i]);
				}
			}
			if ( sem_unlink(SNAME) == -1){
				perror("oss: Error: sighandler(): sem_unlink");
			}
			break;
		// Handle anyother interuptions
		case SIGINT:
			for( i = 0; i < children; i++){
				if(kill(child_pid[i], SIGKILL) == 0){
					while(waitpid(-1, &stat, WNOHANG)== 0);
					printf("Master: %d killed from SIGINT\n", child_pid[i]);
				}
			}
			if ( sem_unlink(SNAME) == -1){
				perror("oss: Error: sighandler(): sem_unlink");
			}
			break;
	}
	// No matter the signal always cleanup
	cleanup();
	exit(0);
}

// Cleanup function
void cleanup(){
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);
	if (file_ptr != NULL){
		fclose(file_ptr);
	}
}
