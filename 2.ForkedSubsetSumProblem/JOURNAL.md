### How to compile:
	1. cd to vong.2/
	2. run 'make'

### Constants:
Default timed interrupt = 10 sec
Timed interrupt for children = 2 seconds

### Subset Sum + printing logic:
1. Use given code to create subset sum table of 0s and 1s
2. From that table, create a mirrored table of 0s, 1s, and 2s
    - Where elements with the value 2 will sum to the number we are looking for.

### Some known loose ends:
1. When child is terminated through use of signals, file pointer will close.
    - While file pointer will be closed, dynamically allocated memory cannot be freed.
        - This could be solve if I make them global but this would cause more trouble.

2. Tokenizer dynamically allocate a `char**`, which will be at the end of the function call.
    - I am able to free the free the token using `free(tokens)`
    - The problem lies with freeing `token[i]`:
        - This cause a 'double free error' although I am not freeing token any where else.
            - An answer I've come up with is, because I only assign `token[i]` to something and never `malloc`, thus, I never actually allocated `token[i]` thus no need to free.

### Vulnerabilities:
1. When testing to see if the child will terminate after 1 sec I used a while(1) loop.
    - Once the terminating signal is received, a function will be called to close any opened files and exits.
    - If a `while(1)` is placed between the `fopen()` and `fprintf()`, then when the signal is sent for the process to terminate, `fclose()` will try to close a random spot in memory which is not allowed and will cause errors to be printed on the screen.

### Bugs:
1. This program cannot process sets of over 80 numbers.
    - While my tokenizer can tokenize the set of numbers dynamically, the bug comes from my `int_array(char**)` function.
        - `int_array(char**)` is supposed to go through all the elements of the tokens and convert it into integer using `atoi()`.
            - When ever the token size is over 80, the for loop to convert every token to an integer ends after the 15th itteration. 
            - Strangely after the 15th itteration the next line of code doesnt execute. Furthermore the next line in the function that calls `int_array()` (which is `processor()`) also doesn't execute.
            - The perror in main() will still execute stating the program exitted successfully.
