# Project 2B: xv6 Scheduler
## Changelog:
- **[update Oct 4]** clarify: when a process created and added to the tail of a queue, this also counts as one "moved to tail".

## Objectives
- To understand code for performing context-switches in the xv6 kernel.
- To implement a basic MLQ scheduler and FIFO scheduling method.
- To create system calls that get/set priority and another syscall to extract process states.

## Overview
In this project, you'll be implementing a simplified multi-level queue (MLQ) scheduler in xv6.

The basic idea is simple. Build an MLQ scheduler with four priority queues; the top queue (numbered 3) has the highest priority and the bottom queue (numbered 0) has the lowest priority. The priority is static in general, so a process running on one privilege level won't be promoted or demoted unless the user invokes a syscall to set its priority. The scheduling method in each of these queues will be round-robin, except the bottom queue which will be implemented as FIFO.

To make your life easier and our testing easier, you should run xv6 on only a single CPU (the default is two). To do this, in your Makefile, replace CPUS := 2 with CPUS := 1.

## Details
You have three specific tasks for this part of the project. However, before starting these tasks, you need first to have a high-level understanding of how the scheduler works in xv6.

Most of the code for the scheduler is quite localized and can be found in kernel/proc.c, where you should first look at the routine scheduler(). It's essentially looping forever and for each iteration, it looks for a runnable process across the ptable. If there are multiple runnable processes, it will select one according to some policy. The vanilla xv6 does no fancy things about the scheduler; it simply schedules processes for each iteration in a round-robin fashion. For example, if there are three processes A, B, and C, then the pattern under the vanilla round-robin scheduler will be A B C A B C ... , where each letter represents a process scheduled within a timer tick, which is essentially ~10ms, and you may assume that this timer tick is equivalent to a single iteration of the for loop in the scheduler(). Why 10ms? This is based on the timer interrupt frequency setup in xv6 and you may find the code for it in kernel/timer.c.

When does the current scheduler make a scheduling decision? Basically, whenever a thread calls sched(). You'll see that sched() switches between the current context back to the context for scheduler(); the actual context switching code for swtch() is written in assembly and you can ignore those details. So, when does a thread call sched()?  You'll find three places: when a process exits, when a process sleeps, and during yield().  

When a process exits and when a process sleeps are intuitive, but when is yield() called?  You'll want to look at trap.c to see how the timer interrupt is handled and control is given to scheduling code; you may or may not want to change the code in trap(). 

Other important routines that you may need to modify include allocproc() and userinit(). Of course, you may modify other routines as well. 

Now to implement MLQ, you need to schedule the process for some time-slice, which is some multiple of timer ticks. For example, if a process is on the highest priority level, which has a time-slice of 8 timer ticks, then you should schedule this process for ~80ms, or equivalently, for 8 iterations.

xv6 performs a context-switch every time a timer interrupt occurs. For example, if there are 2 processes A and B that are running at the highest priority level (queue 3), and if the round-robin time slice for each process at level 3 (highest priority) is 8 timer ticks, then if process A is chosen to be scheduled before B, A should run for a complete time slice (~80ms) before B can run. Note that even though process A runs for 8 timer ticks, every time a timer tick happens, process A will yield the CPU to the scheduler, and the scheduler will decide to run process A again (until its time slice is complete).

### Implement MLQ
Your MLQ scheduler must follow these very precise rules:

- Four priority levels, numbered from 3 (highest) down to 0 (lowest).
- The time-slice associated with priority 3 is 8 timer ticks; for priority 2 it is 12 timer ticks; for priority 1 it is 16 timer ticks, and for priority 0 it executes the process until completion (because it is FIFO).
- Whenever the xv6 10 ms timer tick occurs, the highest priority ready process is scheduled to run.
- The highest priority ready process is scheduled to run whenever the previously running process exits, sleeps, or otherwise yields the CPU.
- You should not trigger a new scheduling event when a new process arrives, wakes, or has its priority modified through setpri(); you should wait until a timer tick to schedule the highest priority process. 
- If there are more than one process on the same priority level, then your scheduler should schedule all the processes at that particular level in a round-robin fashion, except for priority level 0, which will be scheduled using a FIFO basis. For round-robin scheduling, after a process consumes its time-slice it should be moved to the back of its queue. For example, if a process is at the highest priority level, which has a time-slice of 8 timer ticks, then you should schedule this process for 8 ticks before moving to the next process.
- When a timer tick occurs, whichever process was currently using the CPU should be considered to have used up an entire timer tick's worth of CPU, even if it did not start at the previous tick (Note that a timer tick is different than the time-slice.)
- When a new process arrives, it should inherit the priority of its parent process. The first user process should start at the highest priority.
- If no higher priority job arrives and the running process does not relinquish the CPU, then that process is scheduled for an entire time-slice before the scheduler switches to another process.
- Whenever a process wakes or moves to a new priority, it should always be added to the back of that queue with a full new time slice.
- You will not implement any mechanism for starvation for this project.

### Syscalls to get/set priority
As you are implementing MLQ which uses static priority (rather than MLFQ which uses dynamic priority), an additional mechanism to set priority is needed. You'll need to create two syscalls:

- int setpri(int pid, int pri): It will set the priority of a specific process (specified by the first argument pid) to pri. pri should be an integer between 0 and 3. You should check that both pid and pri are valid; if they are not, return -1. When the priority of a process is set, the process should go to the end of the queue at that level and should be given a new time-slice of the correct length. The priority of a process could be increased, decreased, or not changed (in other words, even when the priority of a process is set to its current priority, that process should still be moved to the end of its queue and given a new timeslice). Note that calling setpri() may cause a new process to have the highest priority in the system and thus need to be scheduled when the next timer tick occurs. (When testing your pinfo() statistics below, we will not examine how you account for the case when setpri() is applied to the calling process.)
- int getpri(int pid): It will returns the current priority of the process specified by pid. If pid is not valid, it returns -1. 

### Syscalls to extract scheduling information
Because your MLQ implementations are all in the kernel level, you need to extract useful information for each process by creating this system call so as to better test whether your implementations work as expected. You'll need to create one system call for this project: Create new system calls:

int getpinfo(struct pstat * status)

The argument status is a pointer to a struct pstat to which the kernel is going to fill in the information. This system call returns 0 on success and -1 on failure. If success, some basic information about each process: its process ID, how many timer ticks have elapsed while running in each level, which queue it is currently placed on (3, 2, 1, or 0), its current procstate (e.g., SLEEPING, RUNNABLE, or RUNNING), etc will be filled in the pstat structure as defined

	struct pstat {
	  int inuse[NPROC]; // whether this slot of the process table is in use (1 or 0)
	  int pid[NPROC]; // PID of each process
	  int priority[NPROC]; // current priority level of each process (0-3)
	  enum procstate state[NPROC]; // current state (e.g., SLEEPING or RUNNABLE) of each process
	  int ticks[NPROC][4]; // number of ticks each process has accumulated at each of 4 priorities
	  int qtail[NPROC][4]; // total num times moved to tail of queue (e.g., setpri, end of timeslice, waking)
	};
The code above can be found in ~cs537-10/ta/p2b/pstat.h. Do not change the names of the fields in pstat.h.

## Hints
- Again, most of the code for the scheduler is quite localized and can be found in proc.c; the associated header file, proc.h is also quite useful to examine. To change the scheduler, not too much code needs to be done; study its control flow and then try some small changes.
- As part of the information that you track for each process, you will probably want to know its current priority level and the number of timer ticks it has left.
- It is much easier to deal with fixed-sized arrays in xv6 than linked-lists. For simplicity, we recommend that you use arrays to represent each priority level.
- You'll need to understand how to fill in the structure pstat in the kernel and pass the results to user space and how to pass the arguments from user space to the kernel. - You may want to stare at the routines like int argint(int n, int *ip) in syscall.c for some hints.
- To run the xv6 environment, use make qemu-nox. Doing so avoids the use of X windows and is generally fast and easy. To quit, you have to know the shortcuts provided by the machine emulator, qemu. Type control-a followed by x to exit the emulation. There are a few other commands like this available; to see them, type control-a followed by an h.
- In scheduler(), you may find a control flow like this:
```
acquire(&ptable.lock);
// do some work with ptable...
release(&ptable.lock);
```
We haven't talked about lock in the lecture, but institutively, this lock is trying to prevent multiple processes change ptable at the same time (which can be problematic). You should put your code between lock acquire and release to ensure your code is also protected by this lock.

## The Start Code
We suggest that you start from the source code of xv6, instead of your own code from p1b as bugs may propagate and affect this project. You may refer to p1b's description for the instructions on getting the xv6 source code.

Particularly useful for this project: Chapter 5 in xv6 book.
