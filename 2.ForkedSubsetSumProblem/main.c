#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

// Author: Khanh Vong
// Class: 4760
// Assignment: Project 2
// Description: Using fork() to create child process and solve the sumset-sum problem.
//				With signal handling and timed exits.


// Global that must be closed/kill at termination
FILE* file_ptr = NULL;
pid_t childpid = -1;

// Timer variable (only changed in main)
int master_timer = 10;

//Function prototypes
char* d_readfile(char*, int);
char** tokenize(char*);
void display_array(char **);
void display_subset(int*);
int* int_array(char **);
int get_plength(int*);
int get_length(char **);
void clean_file(char*);
void childs_job(char*, char*, int);
void parents_job(char*, pid_t, int);
int processor(char*, char*, int);
bool subset_sum(int*, int, int, FILE*);
void print_subset(int*, int**, int, int, FILE*, int);
void kill_child(int);
static void child_handler(int);
static int child_setupinterrupt(void);
static int child_setupitimer(void);

// Driver program
int main(int argc, char ** argv){
	// Default arguments
	char* input = "input.dat";
	char* output = "output.dat";

	// Get option if specified
	int opt;
	while((opt = getopt(argc, argv, ":ht:i:o:")) != -1){	
		switch(opt){
			case 'h':
				printf("Usage: logParser [h][i|o] [filename]\n");
				return 0;
				break;
			case 't':
				master_timer = atoi(optarg);
				break;
			case 'i':
				input = optarg;
				printf("Input = '%s'\n", input);
				break;
			case 'o':
				output = optarg;
				printf("Output = '%s'\n", output);
				break;
			case ':':
				printf("No value specified\n");
				return 1;
				break;
			case '?':
				printf("Unknown option: %s\n", optopt);
				return 1;
				break;
		}
	}
	// Get the first line and read the first item to get number of fork needed
	char* f_content = d_readfile(input, 0);
	char** tokens = tokenize(f_content);
	int length = atoi(tokens[0]);
	free(f_content);
	free(tokens);

	// Process data accordingly
	processor(input, output, length);

	perror("Program exit");
	return 0;
}

// Dynamically read file at a specific line
char* d_readfile(char* filename, int line){
	FILE* f_ptr = fopen(filename, "r");
	if(!f_ptr){
		printf("Error opening file '%s' for reading.\n", filename);
		perror("logParse: Error: 'd_readfile()' failed at fopen()");
		exit(EXIT_FAILURE);
	}

	int ch, count = 0;
	int i;
	int buf_size = 80;
	char* str;
	str = malloc(buf_size*sizeof(char));	
	if(!str){
		printf("Error allocating 'str'.\n");
		perror("logParse: Error: 'd_readfile()' failed at malloc():");
		exit(EXIT_FAILURE);
	}

	// Ignore until arived at line
	char ignore[1000];
	for(i = 0; i < line; i++)
		fgets(ignore, sizeof(ignore), f_ptr);

	// Read line
	while(1){
		ch = fgetc(f_ptr);
		if(ch == EOF || ch == '\n'){
			str[count] = '\0';	
			break;
		}
		str[count++] = ch;
		if(count >= buf_size){
			str = realloc(str, sizeof(char)*(buf_size+=40));
			if(!str){
				printf("Error reallocating str.\n");
				perror("logParse: Error: 'd_readfile()' failed at realloc()");
				exit(EXIT_FAILURE);
			}
		}
	}
	fclose(f_ptr);
	return str;
}

// Dynamically tokenize a sring
char** tokenize(char* string){
	int buf_size = 80;
	char** tokens = malloc(buf_size * sizeof(char**));
	if (!tokens){
		perror("logParse: Error: 'tokenize()' failed at malloc{}");
		exit(EXIT_FAILURE);
	}
	
	char* token;
	char* delim = " \t";
	int index = 0;
	token = strtok(string, delim);

	// Store token into tokens
	while(1){
		if( token == NULL){
			break;
		}
		tokens[index++] = token;
		
		if(index >= buf_size){
			tokens = realloc(tokens, sizeof(char)*(buf_size+=40));
			if(!tokens){
				printf("Error reallocating while tokenizing\n");
				perror("logParse: Error: 'tokenize()' failed at realloc()");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, delim);
	}
	// Set last index to NULL as a flag for last element
	tokens[index] = NULL;
	return tokens;
}

// For displaying double pointer
void display_array(char** str){
	int index = 0;
	while(str[index] != NULL){
		printf("%s|", str[index++]);
	}
	printf("\n");
}

// For displaying int pointer
void display_subset(int* subset){
	int i;
	while(subset[i++] != -1){
		printf("%d|", subset[i]);
	}
	printf("\n");
}

// Get the length of a double pointer
int get_length(char** str){
	int index = 0;
	while(str[index] != NULL){
		index++;
	}
	return index;
}

// Convert double pointer into int pointer
int* int_array(char** str){
	int index;
	int length = get_length(str);

	int* int_arr = (int*)malloc((length+1)*sizeof(int));
	if(!int_arr){
		perror("logParse: Error: 'int_array()' failed in malloc()");
		exit(EXIT_FAILURE);
	}
	for( index = 0; index < length; index++)
		int_arr[index] = atoi(str[index]);

	// End of pointer is a number we know we won't have *key*
	int_arr[length] = -1;
	return int_arr;
}
 
// Get length of an int pointer; only works for our specific case
int get_plength(int* arr){
	int length = 0;
	int index =0;

	// Loop until out of range number is found, in this case -1
	while(arr[index++] != -1){
		length++;
	}
	return length;
}

// Job childs proccess
void childs_job(char* input, char* output, int line){
	// Read line
	char* str = d_readfile(input, line);
	char** tokens = tokenize(str);

	// Convert to int pointer to manipulate the numbers
	int* int_arr = int_array(tokens);
	int sum = int_arr[0];
	int sublength = get_plength(int_arr) -1;
	int* subset = (int*)malloc(sublength*sizeof(int));
	if (!subset){
		perror("logParse: Error: 'childs_job()' failed at malloc()");
		exit(EXIT_FAILURE);
	}

	// Set subset
	int i;
	for(i = 0; i < sublength; i++)
		subset[i] = int_arr[i+1];

	// Open file for appending
	file_ptr = fopen(output, "a");
	if(!file_ptr){
		printf("File '%s' could not be opened for writing.\n", output);
		perror("logParse: Error: 'childs_job()' failed at fopen()");
		exit(EXIT_FAILURE);
	}

	
	// Write pid
	fprintf( file_ptr, "%d: ", (int)getpid());

	// Check if sum exists
	bool subset_found = subset_sum(subset, sublength, sum, file_ptr);
	if (subset_found){
		fprintf(file_ptr, "%d.\n", sum);
	}
	else
		fprintf(file_ptr, "No subset of numbers summed to %d.\n", sum);
		
	fclose(file_ptr);
	free(tokens);
	free(str);
	free(int_arr);
	free(subset);

	// Send signal based on whether sum was found or not
	if(subset_found)
		exit(1);
	else
		exit(0);
}

// Jobs for the parent
void parents_job(char* output, pid_t childpid, int result){
	file_ptr = fopen(output, "a");
	if(!file_ptr){
		printf("File '%s' could not be opened for writing.\n", output);
		perror("logParse: Error: 'parents_job()' failed at fopen()");
		exit(EXIT_FAILURE);
	}
	if(result)
		fprintf(file_ptr, "%d: Sum found\n", childpid);
	else
		fprintf(file_ptr, "%d: Sum not found\n", childpid);
	fclose(file_ptr);
}

// Delete every thing in a file
void clean_file(char* input){
	file_ptr = fopen(input, "w");
	if(!file_ptr){
		printf("File '%s' could not be opened for cleaning.\n", input);
		perror("logParse: Error: 'clean_file()' failed at fopen()");
		exit(EXIT_FAILURE);
	}
	fprintf(file_ptr, "");
	fclose(file_ptr);
}

// Proccess data and create subtasks
int processor(char* input, char* output, int lines){
	// Empty output file
	clean_file(output);
	int x = 0;
	int result = 0;
	int i;

	// Set master timer
	signal(SIGALRM, (void (*)(int))kill_child);
	alarm(master_timer);

	// Fork on every iteration
	for(i = 0; i < lines; i++){
		childpid = fork();
		if (childpid < 0){
			perror("logParse: Error: 'processor()' failed to fork()");
			return 1;
		}
		if (childpid == 0){ // Child will do this
			if(child_setupinterrupt() == -1){
				perror("logParse: Error: 'processor()' failed to set up handler for SIGPROF");
				return 1;
			}
			if(child_setupitimer() == -1){
				perror("logParse: Error: 'processor()' failed to set up the ITIMER_PROF interval timer");
				return 1;
			}
			childs_job(input, output, i+1);
		}
		else{ // Parent will do this
			int stat;
			wait(&stat);
			if(WIFEXITED(stat))
				printf("Child %d exited with status %d\n", i+1, WEXITSTATUS(stat));
		}
	}
	return result;
}

// Subset sum problem
bool subset_sum(int* set, int n, int sum, FILE *file_ptr){
	bool subsetA[n+1][sum+1];
	int i, j;
	int** subsetB = (int**)malloc((n+1)*sizeof(int*));
	if (!subsetB){
		perror("logParse: Error: 'subset_sum()' failed at malloc()");
		exit(EXIT_FAILURE);
	}
	
	for(i = 0; i <= n; i++){
		subsetB[i] = (int*)malloc(sizeof(int)*(sum+1));
		if (!subsetB[i]){
			perror("logParse: Error: 'subset_sum()' failed at malloc() in for loop allocation");
			exit(EXIT_FAILURE);
		}
	}
	
	// Set initial values
	for(i = 0; i <= n; i++)
		subsetA[i][0] = true;
	for(i = 1; i <= sum; i++)
		subsetA[0][i] = false;
	
	// Change value based on the subset values
	for(i = 1; i <= n; i++){
		for(j = 1; j <= sum; j++){
			bool sum_able1 = subsetA[i-1][j];
			bool sum_able2 = false;
			if((j - set[i-1]) >= 0)
				sum_able2 = subsetA[i-1][j-set[i-1]];
			if(sum_able2)
				subsetB[i][j] = 2;
			else if (sum_able1)
				subsetB[i][j] = 1;
			else
				subsetB[i][j] = 0;
			subsetA[i][j] = sum_able1 || sum_able2;
		}
	}
	// Output subset we found
	print_subset(set, subsetB, n, sum, file_ptr, 0);
	
	// Freeing b
	for(i = 0; i <= n; i++){
		free(subsetB[i]);
	}
	free(subsetB);
	return subsetA[n][sum];
}

// Recursively find the subset sum and write it to a file
void print_subset(int* set, int** b, int n, int sum, FILE *file_ptr, int count){
	if(b[n][sum] == 1){
		print_subset(set, b, n - 1, sum, file_ptr, count);
	}
	else if(b[n][sum] == 2){
		print_subset(set, b, n - 1, sum - set[n-1], file_ptr, count + 1);
		if (count == 0)
			fprintf( file_ptr, "%d = ", set[n-1], file_ptr);
		else
			fprintf( file_ptr, "%d + ", set[n-1], file_ptr);
	}
}

// Handler for sending signal to kill child process; *called from parent
void kill_child(int s){
	printf("Proccess has reached its time limit of %d second(s). Terminating child proccess and exiting..\n", master_timer);
	kill(childpid, SIGPROF);
	exit(1);
}

// Handler for closing files and killing child
static void child_handler(int s){
	if(file_ptr != NULL){
		fprintf(file_ptr, "%d: Killed from child. No valid subset found after 1 second.\n", (int)getpid());
		fclose(file_ptr);
	}
	exit(1);
}

// Timer interrupt
static int child_setupinterrupt(void){
	struct sigaction act;
	act.sa_handler = child_handler;
	act.sa_flags = 0;
	return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL));
}

static int child_setupitimer(void){
	struct itimerval value;
	value.it_interval.tv_sec = 1;
	value.it_interval.tv_usec = 0;
	value.it_value = value.it_interval;
	return (setitimer(ITIMER_PROF, &value, NULL));
}
