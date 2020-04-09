How to compile:
	- Run 'make' in vong.3 directory.

Description: OSS(master.c) will launch s processes which will execute dostuff(child.c).
OSS is to keep s processes active at any given time during its execution. In OSS, theres
a simulated clock. dostuff will set a termination time, if time has passed then terminate.
At the termination of a child, OSS will write to a log file indicating the child has finished
executing. At the same time the OSS will launch a new child to maintain s processes at all time.
Because there are many child processes, semaphores and a critical section must be introduce so
that no process can modify the shared memory while another is accessing it.

USAGE: oss [[-s child_processes][-l logfile][-t time][-p process_cap]] [-h]
	-s: Number of child processes active at all time (default 5).
	-l: File to output child's termination result (default 'oss.log').
	-t: Seconds until OSS will automatically terminate (default 5).
	-p: Maximum process allowed to run (default 100). <-- Added feature

Some explainations:
	- Only 96 lines written to the log file:
		- By default, 100 process will be allowed to launch. Right before the 100 process are
		created, the OSS will start its termination process. At that point only 95 processes 
		had run but 100 process has been created. However, after the parent has finished 
		processing the terminated child's information the parent will "empty" the message box,
		which lets a waiting process into the critical section to execute. Thus, at the end of 
		termination 96 processes will have executed.
			- Given t = total processes created, s = number of process active at any time,
			n = total processes executed.
					n = t - (s - 1)
	- Occasional starvation of other processes:
		- No scheduling algorithm was implemented for this assignment. So, some occasions there
		are noticable starvations of some child process(es).

For testing:
	- To terminate from simulated clock.
		- ./oss -s 1
	- To terminate from maximum created.
		- ./oss
	- To terminate from real clock.
		- Add "while(1);" in child.c or anywhere in master.c after the signal has been set up.


GIT LOG:

commit 9ca92ef36c24b1365b57956b630f039ebbf4b5a7
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Oct 4 21:44:59 2019 -0500

    Makefile updated

commit 88be59eb680db35fe4c4c3691d54747f53f4716d
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Oct 4 21:44:12 2019 -0500

    README updated

commit 90219359b6fdd6ff80094510c5b029bb35601669
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Oct 4 21:43:09 2019 -0500

    Comments added

commit 36521055cc297a6045b7fe0694887eee632f4b7b
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Oct 2 00:14:19 2019 -0500

    Took out sighandler in child. Combine semaphore into struct. Allow child to die gracefully

commit 0d32e7ad54d1efda95091c69ce58c9fa55984815
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Sep 29 01:39:35 2019 -0500

    Process maintainer, file writing added. Need to address orphan issue

commit 208aa5df2f8197987ec973537f43d884ca64e6d3
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Sep 28 21:51:11 2019 -0500

    Semaphores, crit. sect., mutex implemented. File writing needed

commit 7869d9e7f3c28a7782c0057315182e48aecc4404
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Sep 28 14:47:44 2019 -0500

    Simulated clock added, new process maintainer needed

commit 023912d2824fbf44d8f0fce4ebbddd371c7f43c8
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Sep 28 00:23:08 2019 -0500

    Spawning process added, semaphores needed

commit ea44915dcf908380c05ce4061d19603639198d7e
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Sep 27 16:41:42 2019 -0500

    Every child process now return their p#

commit 316ee39d7c158224562c4eec2b3708045d085067
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Sep 27 14:35:01 2019 -0500

    shmry_t added for shared memory

commit c207cdfa447f69ceefb7ddeda89a2d0035e111ae
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Sep 27 00:56:49 2019 -0500

    Implemented reading shared memory

commit 11faf7ad30966469eb3a0b579744498757383362
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Thu Sep 26 23:56:52 2019 -0500

    Child executable created

commit 1e19a07ff69084dd0caa8e583d9ccd6aacc20d58
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Sep 25 19:33:34 2019 -0500

    Commandline option, child process, master timer added

commit 0d0255964b287e359d95ce9a308bd0ec8501b69c
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Sep 25 17:59:26 2019 -0500

    Initial commit
