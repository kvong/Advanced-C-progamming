## CS4760:		Operating Systems
## Author:		Khanh Vong
## Project 6:	OS Simulator for Mem. Management with FIFO and LRU replacement policy.

-------------------------------------------------------------------------------

### Instruction for compiling:

	1. Change directory 'cd src'
	2. Run 'make'

-------------------------------------------------------------------------------

### Usage:
1. `./oss` will run OSS with FIFO replacement policy (DEFAULT)

    Options:
        -f: Run with FIFO replacement policy.
        -l: Run with LRU replacement policy.
        -h: Usage message.

-------------------------------------------------------------------------------

### Description:
- OSS will launch spawning children at random times. Child processes run concurrently with eachother but critical section is protected by semaphores and communication between children and parent is happening through the use of message queues.

- A wait queue is created for blocked processes waiting for its request to be granted.

-------------------------------------------------------------------------------

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Dec 1 00:29:18 2019 -0600

    Stats implenented

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 30 20:18:18 2019 -0600

    LRU replacement policy added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 30 15:52:27 2019 -0600

    ReferenceByte Updater added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sat Nov 30 10:52:19 2019 -0600

    FIFO Replacement policy added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Thu Nov 28 17:58:25 2019 -0600

    Frame/Page communication error fixed

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Wed Nov 27 20:28:52 2019 -0600

    Page/Frame interaction with processes added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Tue Nov 26 18:59:51 2019 -0600

    Page/Frame table added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Sun Nov 24 20:25:06 2019 -0600

    Semaphores and Message queue added

Author: khanh vong <vong@hoare7.cs.umsl.edu>
Date:   Tue Nov 19 23:26:40 2019 -0600

    First Init; Proj.3 imported
