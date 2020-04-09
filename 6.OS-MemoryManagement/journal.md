###############################################################################
	CS4760:		Operating Systems
	Author:		Khanh Vong
	Project 6:	OS Simulator for Mem. Management with FIFO and LRU replacement
				policy.
###############################################################################
-------------------------------------------------------------------------------

Instruction for compiling:
	1. Change directory 'cd src'
	2. Run 'make'

-------------------------------------------------------------------------------

Usage:
	1. './oss' will run OSS with FIFO replacement policy (DEFAULT)
		Options:
			-f: Run with FIFO replacement policy.
			-l: Run with LRU replacement policy.
			-h: Usage message.

-------------------------------------------------------------------------------

Description:
	- OSS will launch spawning children at random times. Child processes
	run concurrently with eachother but critical section is protected by
	semaphores and communication between children and parent is happening
	through the use of message queues.

	- A wait queue is created for blocked processes waiting for its request
	to be granted.

-------------------------------------------------------------------------------

commit eef32c9809263c942d652cba045afac6b373439f
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Dec 1 00:29:18 2019 -0600

    Stats implenented

commit 242a788f6ddc1b6c7ddc1bd112744aaa65320df4
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 30 20:18:18 2019 -0600

    LRU replacement policy added

commit 542245b30074c77f85af54c3b4717aaa8002f37b
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 30 15:52:27 2019 -0600

    ReferenceByte Updater added

commit 6dff8841764d59af20c2c4b2296ed7e38b478559
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 30 10:52:19 2019 -0600

    FIFO Replacement policy added

commit 5ebde55b029f8fd097db6e8323f992ddfb696d22
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Thu Nov 28 17:58:25 2019 -0600

    Frame/Page communication error fixed

commit 4af9408b57341679761f6ecf4baaaae887a1ba5d
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Nov 27 20:28:52 2019 -0600

    Page/Frame interaction with processes added

commit a367ccfb4357161e599ea7717c5cedc3ca63a983
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Tue Nov 26 18:59:51 2019 -0600

    Page/Frame table added

commit eac49bf8256c3d07330e1eb2c2adc3b3aa1af3e7
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Nov 24 20:25:06 2019 -0600

    Semaphores and Message queue added

commit 3bd6ae804561bde9c1b4ccd17a1581a9c17db6cb
Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Tue Nov 19 23:26:40 2019 -0600

    First Init; Proj.3 imported
