**CS4760:**	    Operating Systems

**Project 4:** 	OS Simulator with Scheduling Algorithm

**Author:**		Khanh Vong

### Instruction for compiling:

	1. Change directory `cd src`
	2. Run `make`

### Description:
- This is an OS simulator that manages multiple child processes using a scheduling algorithm. This simulator also takes into account the priority of processes by using an array of queues (highest to lowest). Furthermore, child processes also simulate the different behavior that a process can perform such as using its entire time quantum, interruption, blocking, and termination.

- Scheduling Algorithm:
    1. All process starts at the highest priority queue.
    2. A process is moved 1 level lower in priority if:
        A. The process wait time exceeds a specific time threshold.
        B. The process wait time exceeds the average wait time of a constant k times the average wait time of the processes 1 tier lower in priority.
    3. If the process is rescheduled at the same priority a few times then it will be moved to a lower priority. (My personal touch I added to make things interesting)

- Random behavior simulated in child processes:
    - Every time the child is shedule it will roll a number:

        0 = Child process will terminate (10% of the time)
        1 = Child process will run for entire time quantum (30% of the time)
        2 = Child process will wait for an event that will last s time (30% of the time)
        3 = Child process will run for a bit an get interupted (30% of the time)

### Personal Notes:
- Might have been unintended but through this project I can see that srand is not completely random. If multiple children has the same seed then they will have the same random number generated.
- I have both printf and fprintf printing the same thing so I can see what happened when something breaks.
    - Because I don't know how to use gdb...
- Added a 3rd condition for enqueuing to keep a process from having the same priority for too long.
- I set nanosecond bound to be 1000 because its hard to check if clock is updating like its supposed to with million and billions.

Challenges:
1. When I was implementing the round robin sheduling I decided not to use a queue to my processes. Instead I used an array. This worked fine until I had to implement priorities for each process. To convert what I had queues took a good chunk of my time. This could have
been avoided if I only implement the queue instead of the array.
2. Once I has multi-layered round robing sheduling, it was difficult to change to a different scheduling algorithm. I couldn't figure out how average wait time was supposed to be calculated.  It took a while to realize that calculating average wait time only needed to be done when we need to enqueue. This made my job much easier as I was trying to update the average at every tik of the clock.

Maximizing throughput:
	- I found that processes are executed quicker when they move down in priority regularly.
	- If I set alpha = 0, beta = 0, and the wait threshold to 0:0 then every process will move down in priority after executing (except maybe the first process with wait time 0:0). This causes the number of dequeue to be highest on the lowest priority and lowest on the highest priority.

Implementation:
- Prerequisites:
    - 3 Queues (high to low in priority).
    - 2 Message queues:
        - 1 for sending messages to child processes.
        - 1 for receiving messages from child processes.
        - Probably could have achieved the same result with a single message queue.
    - Array of 18 process blocks in shared memory.
    - Process control table local to OSS.
- Actual Implementation:
    - OSS:
        - Launch initial process
            - Create a process control block.
            - Update process table.
            - Equeue to highest priority.
        - Set random spawn time
        Go into a loop.
            - Dequeue process with highest priority.
                - Send message to let it work.
                - Wait to receive that process has finished.
                - Determine to enqueue or not.
                    - If enqueue, where?
            - If spawn time is met, spawn a process.
                - Create a process control block.
                - Update process table.
                - Equeue to highest priority.
                - Set next spawn time
            - Increment clock.
    - Child:
        - Go into loop:
            - Wait for message from parent.
            - If received a message, roll a random action.
            - Send message back to parent, informing parent of action.
            - Exit loop when random action is to terminate.



Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Thu Oct 24 00:34:07 2019 -0500

    Enqueue limit and bound for waiting time added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Oct 23 20:23:15 2019 -0500

    Scheduling algorithm added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Oct 23 16:46:35 2019 -0500

    Block queue added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Oct 23 16:45:28 2019 -0500

    Block queue added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Oct 23 00:22:32 2019 -0500

    Child random variable added; Zombie bug fixed in master

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Tue Oct 22 00:19:03 2019 -0500

    Implemented log-writing

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Mon Oct 21 22:38:08 2019 -0500

    Multilevel feedback queue added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Mon Oct 21 13:46:23 2019 -0500

    Properly implemented round robin

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Mon Oct 21 01:48:28 2019 -0500

    Round robin scheduling with single queue; need to address descrepacy

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Oct 20 14:53:04 2019 -0500

    Round robbin scheduling added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Oct 19 21:13:40 2019 -0500

    Multi-processes single queue added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Oct 19 17:28:01 2019 -0500

    Single message queue added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Oct 18 15:16:41 2019 -0500

    File structure changed

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Oct 12 00:40:05 2019 -0500

    First commit
