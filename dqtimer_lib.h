/**************************************************************************************************************
 * Software Timers with Delta list queue
 * -------------------------------------
 *
 * In embedded systems a base timer uses hardware timer with a free-running counter updated in ISR
 * A scheduler uses this counter to execute programs from various queue lists
 *
 * Single timer resource is not possible to be used by multiple processes or threads especially in networking
 * Software timers which uses delta list queue and one timer resource can be used.
 * Another approach is to use the timer wheel concept which uses hash tables (tradeoff - increased speed to more memory)
 *
 * This example is to show the implementation of delta list timers in linux environment.
 * The insertion,deletion takes O(n),O(1) time respectively.
 * Insertion to the list can be at any location based on calculated "delta time". Hence, worst case time is O(n).
 * Deletion is always O(1)
 *
 * Compilation consideration
 * -------------------------
 * -lpthread, -lrt options are used to compile this test code
 *
 * The code can be modularized to
 *
 * 1) The base timer functionality - sourced from (http://courses.cs.vt.edu/~cs5565/spring2012/projects/project1/posix-timers.c)
 * 2) Software timers library (Delta list queue)
 *
 * Base timer
 * ----------
 * Signals are used to implement a base timer using POSIX threads.
 *
 * Rationale: when using normal Unix signals (SIGALRM), the signal handler executes
 * in a signal handler context.  In a signal handler context, only async-signal-safe
 * functions may be called.  Few POSIX functions are async-signal-safe.
 *
 * Instead of handling the timer activity in the signal handler function,
 * we create a separate thread to perform the timer activity.
 * This timer thread receives a signal from a semaphore, which is signaled ("posted")
 * every time a timer signal arrives.
 *
 * Software timers library
 * -----------------------
 * The 4 API calls are provided to user
 * 1) int dq_timer_init(int timer_resolution);
 * 2) void dq_timer_deinit();
 * 3) int dq_timer_create(int timer_interval, timer_callback_t cb, void *cb_ctx, void **timer_obj);
 * 4) int dq_timer_delete(struct dq_timer *newTimer);
 *
 *************************************************************************************************************/

#ifndef __DQ_TIMER__
#define __DQ_TIMER__

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

#define FALSE	0
#define TRUE	1

#define _TEST_CODE_ 


typedef void (*timer_callback_t) (void *);

/* Software Timer Data structure */
struct dq_timer {
    int timer_interval;
    int timer_remaining;
    timer_callback_t cb;
    void *cb_ctx;
    struct dq_timer *next;
};

/* Base timer Data structure */
struct dq_timer_desc
{
    struct dq_timer *head;
    pthread_mutex_t lock;

    struct sigaction sa;
    struct itimerval timer;

    int timer_elapsed;
    pthread_t thread_id;
    pthread_mutex_t thr_mutex;
    pthread_cond_t thr_cond;
    int schedule;
    int exit; /* A flag to stop the thread gracefully.*/
};

sem_t demo_over;	/* a semaphore to stop the demo when done*/

void *cb_context;

void timer_func(void *); /* Software timer callback function*/
void print_timers(void);

/* API function calls*/
int dq_timer_init(int timer_resolution);
void dq_timer_deinit(void);
int dq_timer_create(int timer_interval, timer_callback_t cb, void *cb_ctx, void **timer_obj);
int dq_timer_delete(struct dq_timer *newTimer);


#endif /*__DQ_TIMER__*/
