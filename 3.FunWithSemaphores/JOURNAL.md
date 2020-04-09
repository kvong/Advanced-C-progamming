### How to compile:
	- Run 'make' in vong.3 directory.

### Description: OSS(master.c) will launch s processes which will execute dostuff(child.c).
OSS is to keeps *s* processes active at any given time during its execution. In OSS, theres a simulated clock. dostuff will set a termination time, if time has passed then terminate.  At the termination of a child, OSS will write to a log file indicating the child has finished executing. At the same time the OSS will launch a new child to maintain *s* processes at all time. Because there are many child processes, semaphores and a critical section must be introduce so that no process can modify the shared memory while another is accessing it.

### USAGE: 
oss {{-s child\_processes}{-l logfile}{-t time}{-p process\_cap}} {-h}
    -s: Number of child processes active at all time (default 5).
	-l: File to output child's termination result (default 'oss.log').
	-t: Seconds until OSS will automatically terminate (default 5).
	-p: Maximum process allowed to run (default 100). <-- Added feature

### Some explainations:
- Only 96 lines written to the log file:
    - By default, 100 process will be allowed to launch. Right before the 100 process are created, the OSS will start its termination process. At that point only 95 processes had run but 100 process has been created. However, after the parent has finished processing the terminated child's information the parent will "empty" the message box, which lets a waiting process into the critical section to execute. Thus, at the end of termination 96 processes will have executed.
        - Given t = total processes created, s = number of process active at any time,
			n = total processes executed.
					n = t - (s - 1)
	- Occasional starvation of other processes:
		- No scheduling algorithm was implemented for this assignment. So, some occasions there are noticable starvations of some child process(es).

### For testing:
- To terminate from simulated clock.
    - ./oss -s 1
- To terminate from maximum created.
    - ./oss
- To terminate from real clock.
    - Add "while(1);" in child.c or anywhere in master.c after the signal has been set up.


### GIT LOG:

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Fri Oct 4 21:44:59 2019 -0500

    Makefile updated

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Fri Oct 4 21:44:12 2019 -0500

    README updated

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Fri Oct 4 21:43:09 2019 -0500

    Comments added

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Wed Oct 2 00:14:19 2019 -0500

    Took out sighandler in child. Combine semaphore into struct. Allow child to die gracefully

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Sun Sep 29 01:39:35 2019 -0500

    Process maintainer, file writing added. Need to address orphan issue

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Sat Sep 28 21:51:11 2019 -0500

    Semaphores, crit. sect., mutex implemented. File writing needed

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Sat Sep 28 14:47:44 2019 -0500

    Simulated clock added, new process maintainer needed

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Sat Sep 28 00:23:08 2019 -0500

    Spawning process added, semaphores needed

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Fri Sep 27 16:41:42 2019 -0500

    Every child process now return their p#

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Fri Sep 27 14:35:01 2019 -0500

    shmry_t added for shared memory

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Fri Sep 27 00:56:49 2019 -0500

    Implemented reading shared memory

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Thu Sep 26 23:56:52 2019 -0500

    Child executable created

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Wed Sep 25 19:33:34 2019 -0500

    Commandline option, child process, master timer added

#### Author: khanh vong <vong@hoare7.cs.umsl.edu>
#### Date:   Wed Sep 25 17:59:26 2019 -0500

    Initial commit
