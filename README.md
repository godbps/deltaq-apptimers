# deltaq-apptimers

Software Timers using Delta list queue
======================================

Introduction  
------------
In embedded systems a base timer uses hardware timer with a free-running counter updated in ISR. A scheduler uses this counter to execute programs from various queue lists.
Single timer resource is not possible to be used by multiple processes or threads especially in networking applications. Software timers which use delta list queue and one timer resource can be used. Another approach is to use the timer wheel concept which uses hash tables (tradeoff - increased speed to more memory).

This example is to show the implementation of delta list timers in Linux environment.
The insertion, deletion takes O(n),O(1) time respectively. 
Insertion to the list can be at any location based on calculated "delta time". Hence, worst case time is O(n). Deletion is always O(1).

Compilation consideration
-------------------------
 -lpthread, -lrt options are used to compile this test code


Code architecture
-----------------
The code is modularized to -
 
 1) The base timer functionality - sourced from (http://courses.cs.vt.edu/~cs5565/spring2012/projects/project1/posix-timers.c)
 
 2) Software timer library (Delta list queue) (dqtimer_lib.c, dqtimer_lib.h)
 
 3) Test application (dqtimer.c)
 
 
Base time (Extracted Info from Source file - posix-timers.c)
------------------------------------------------------------
Signals are used to implement a base timer using POSIX threads.

Rationale: when using normal Unix signals (SIGALRM), the signal handler executes in a signal handler context.  In a signal handler context, only async-signal-safe functions may be called.  

Instead of handling the timer activity in the signal handler function, we create a separate thread to perform the timer activity. This timer thread receives a signal from a semaphore, which is signaled ("posted") every time a timer signal arrives.



Software timer library
----------------------
4 API calls are provided to user -

(1) int dq_timer_init(int timer_resolution)

(2) void dq_timer_deinit(void)

(3) int dq_timer_create(int timer_interval, timer_callback_t cb, void * cb_ctx, void ** timer_obj)

(4) int dq_timer_delete(struct dq_timer * newTimer)


Test application
----------------
(1) Creates software timers (1,2,3,4) with different time intervals (in mS) [100,200,50,135]

(2) These timers are sorted during insertion with their timer_remaining variable set to relative time of expiry of the timer ahead of it in the list

(3) So the values of timer_remaining would show at the beginning (considering no time elapsed) as [50, 50, 35, 100] -> expiry order of timers = (3,1,4,2)

(4) A single call back function is used for all timers

(5) The call back function prints the context values to exhibit the expiry order

(6) With the last timer expiry the application is closed


Result (executable binary generated after compilation ==> dqtimer)
------------------------------------------------------------------
Execute the binary
./dqtimer

The output shows the –

(1) Timer addition sequence

(2) Sorting of timers based on interval

(3) Delta time (remaining) for the timer to run relative to the timer ahead of it [smaller interval]

(4) Timer expiry sequence


Development Environment
-----------------------
(1) Development system – Ubuntu 10.04 (x86)

(2) Compiler – gcc
