###############################################################################
	CS4760:		Operating Systems
	Project 5:	OS Simulator with Deadlock Avoidance(DLA) Algorithm
	Author:		Khanh Vong
###############################################################################
-------------------------------------------------------------------------------

Instruction for compiling:
	1. Change directory 'cd src'
	2. Run 'make'

-------------------------------------------------------------------------------

How to run:
	1. './oss' will run OSS with shareable resources.
		- Note: Using shareable memory means that DLA will ignore the
		resource. By doing a partial DLA we cannot guarantee that there
		will be no deadlock. We can however plan ahead and set up a 
		recovery process (see 'How deadlocks are handled' for more info).
	2. './oss x' will run OSS with no shareable resources. Where x is
	any argument.
		- Note: This will have no deadlocks because DLA is applied to
		all resources.

-------------------------------------------------------------------------------

Description:
	- This OS simulator similates the random request and release of
	resources done by different processes. The algorithm we use is 
	Deadlock Avoidance. 

	- OSS utilized an active and wait queue. The wait queue has higher
	priority than the active queue.
		- When a process is spawn it is placed into the active queue.
		If the wait queue is empty or Deadlock Avoidance detected
		that processes in the wait queue in an unsafe state, then OSS
		will look into the active queue and process another request.

	- OSS ensures that there's always 15-25% shareable resources.
		- Shareable resource are  ignored by Deadlock Avoidance.

-------------------------------------------------------------------------------

How deadlocks are handled:
	- When running with shareable resources, deadlocks are bound to occur
	because the shared resources will be ignored while performing DLA.

	- Deadlock recovery is done by forcing all processes to give up all of
	their resources and basically reloading them back into the system.
		- This is done simply by deallocating all resources and have the
		process re-request the source that caused the deadlock in the first
		place.

-------------------------------------------------------------------------------

Basic Scheduling:
		- Wait Queue:
			- High priority
			- When a blocked process receives its requested resource it is
			placed into Active Queue so it can choose a new option.
				- Else it will be moved to the end of the Wait Queue.

		- Active Queue:
			- Low priority
			- When a process is not waiting for any resources, it will wait
			for its turn to run, where it will select a new option to be 
			process.
			- Unsafe request will be moved to Wait Queue. That process is
			now considered blocked.
				- Else it will be moved to the end of the Active Queue.

-------------------------------------------------------------------------------

Implementation:
	- OSS:
		Loop:
			- Launch process at random time.
			- Look into all blocked processes in wait queue.
				- Determine which request is safe to run.
			- If all blocked requests are unsafe, look into active queue.
				- For every process in the active queue.
					- Send message via message queue.
					- Wait to receive message for request/release/termination
					action.
					- Process that message.
						- Put in wait queue if it's a request and cannot be
						granted.

	- Child:
		- Loop:
			- Pick an action option.
			- Wait for message from OSS to run.
				- When received, send action option to OSS for processing.
		
-------------------------------------------------------------------------------
		
Person Notes:
	- I tried using semaphores but using semaphores I couldn't control
	which process was allowed to run without additional implementations. 
	In the end, my implementation with semaphores look relatively close 
	to a message queue so I decided to remove all semaphores and add 
	message queues.
		- Using message queue for IPC, I was able to minimize the amount
		checks I had to perform in child.c. Most of the processing is done
		in OSS which is the way it's supposed to be. Also with the use of 
		the active and wait queue this assignment became marginally easier.
		Because OSS can now practically schedule safe process to run and 
		block unsafe processes until they are deemed safe.

	- When process bound is reached, OSS will start its cleanup process.
	I had a problem where some child processes will try to access the
	message queue and shared memory even after its been deleted.
		- Solution: Send an impossible message that would represent OSS
		is terminating. Child will basically forget everything it's doing
		and simply terminate.
	
	- When running with shareable resources expect lots of deadlocks. Typically
	the recovery method will take care of the deadlocked state but there is are
	cases where OSS keeps on deadlocking until time runs out.

	- I notice that whenever hoare is running slow SIGALARM will be raised
	before the OSS can finish spawning 100 processes and terminate properly.

-------------------------------------------------------------------------------

-------------------------------------------------------------------------------

commit 9d0199197310009f6c2386451d2c5bf228edd318
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Nov 10 11:45:30 2019 -0600

    README Updated

commit c75820a0b44bf3ea0bbcbf91f43be8698befac16
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Nov 10 00:33:19 2019 -0600

    15-25% shareable implemented

commit c5681cb60be7a1f1ac5988a4fbdbfb7b3543d543
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 9 22:10:45 2019 -0600

    Semaphore replaced with Message Queue

commit fe14b2222d33fe5db56c97b90b1f74faaa203ad4
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 9 12:37:55 2019 -0600

    Semaphore with DLA added, active queue needed

commit f8b65520aacdbfb12fadc362f9b5801e5690f57e
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Nov 6 12:19:39 2019 -0600

    Tracker needed

commit 77e1b38c781785d2d8dd3b13dc7ebda1fb7b615c
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Mon Nov 4 00:34:53 2019 -0600

    DL algorithm added, bugs need fixing

commit 1f98b2f5fe782204dff08b147ee16d7d1d0f93ef
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 2 21:42:43 2019 -0500

    Resource tables, shared memory and semaphore added

commit bb8d1f1bf2898c7e3ab4c8f4ad7fa7bd634f8e1c
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 2 18:29:02 2019 -0500

    Structure and table created

commit 8eac727c212b4e28b024f217f1e87cff103c7b1a
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Fri Nov 1 19:46:41 2019 -0500

    Structure added. Descriptor created

commit 8af262680571a8ff42c51b0893a4c8db60c5ec73
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Mon Oct 28 17:18:23 2019 -0500

    First commit, file organizer added
