/*
 * delta list queue library and static functions
 */

#include "dqtimer_lib.h"

/* Global data structure to keep timer data and pointer to software time datastructure */
static struct dq_timer_desc *dqt = NULL;

/* variables needed for base timer */
static sem_t timer_sem;		/* semaphore that's signaled if timer signal arrived */
static int timer_stopped = FALSE;	/* non-zero if the timer is to be timer_stopped */

/* Static functions */
static void ds_timer_add_to_queue(struct dq_timer *newTimer);
static void timer_signal_handler(int signum);
static void *timer_schedule_func(void *obj);
static void set_periodic_timer(long delay);

/* 
 * Timer signal handler.
 * On each timer signal, the signal handler will signal a semaphore.
 */
static void timer_signal_handler(int signum) 
{
	/* Set the schedule variable to TRUE to check expired timers from delta queue */
	pthread_mutex_lock(&dqt->thr_mutex);
	dqt->schedule = TRUE;/*Enable scheduler to check if timer expired*/
	--dqt->timer_elapsed;/*Reduce count by 1*/
	pthread_mutex_unlock(&dqt->thr_mutex);

	/* called in signal handler context, we can only call 
	 * async-signal-safe functions now!
	 */	
	sem_post(&timer_sem);	/* the only async-signal-safe function pthreads defines */

}

/* 
 * Base Timer thread.
 * This dedicated thread waits for posts on the timer semaphore.
 * For each post, the soft timer at the head of the dq list is checked for expiry
 * If it expired, timer is removed from the list and its call back function is called.
 * 
 * This ensures that the call back functions are not called in a signal context.
 */
static void *timer_schedule_func(void *obj) 
{
	struct dq_timer_desc *dqtObj = (struct dq_timer_desc *) obj;
	struct dq_timer *tmp = dqtObj->head;
	timer_callback_t callback_func;
	
	while (!timer_stopped) {		
		int rc = sem_wait(&timer_sem);/* retry on EINTR */
		
		if (rc == -1 && errno == EINTR)
		    continue;		
		if (rc == -1) {
		    perror("sem_wait");
		    exit(-1);
		}		
		//printf("dqt->timer_elapsed = %d In Scheduler thread\n",dqt->timer_elapsed);
		tmp = dqtObj->head;
		if(NULL != tmp)	{
			if(TRUE == dqtObj->schedule) {
				/* Check the elapsed timers from the delta queue */
				pthread_mutex_lock(&dqtObj->thr_mutex);
				dqtObj->schedule = FALSE;
				//printf("dqtObj->timer_elapsed = %d,  In Scheduler thread\n",dqtObj->timer_elapsed);
				
				if(0 >= dqtObj->timer_elapsed) {
					callback_func = tmp->cb; /* Assign the call back function to call after unlocking and deleting */
					cb_context = tmp->cb_ctx;										
					pthread_mutex_unlock(&dqtObj->thr_mutex);
					dq_timer_delete(tmp);
					/* call back timer func from delta queue, can do anything it wants*/
					(callback_func)(cb_context);
				}
				else {
					tmp->timer_remaining = (tmp->timer_remaining - dqtObj->timer_elapsed);
					pthread_mutex_unlock(&dqtObj->thr_mutex);
				}			
			}
		}

		if(TRUE == dqtObj->exit) {			
			sem_post(&demo_over);	/* signal main thread to exit */
		}
	}
	return 0;		
}

/* 
 * Set a periodic timer as Base timer 
 */
static void set_periodic_timer(long delay) 
{
	struct itimerval tval = { 
		/* subsequent firings */ .it_interval = { .tv_sec = 0, .tv_usec = delay }, 
		/* first firing */       .it_value = { .tv_sec = 0, .tv_usec = delay }};

	setitimer(ITIMER_REAL, &tval, (struct itimerval*)0);
}

/*
* Add to delta queue
*/
static void ds_timer_add_to_queue(struct dq_timer *newTimer)
{
	/*Check if the timer wheel is empty. If so make current node as the first node.
	* Else read the timerElapsed value and compare with new timerInterval 
	* Insert the new timer node at appropriate location in the ascending sorted list
	* based on timer interval
	*/
	struct dq_timer *tmp = dqt->head;
	struct dq_timer *prev = NULL;
	int timer_total = 0;

	//printf("In -> ds_timer_add_to_queue\n");

	/* First addition to the delta list queue */
	if(NULL == tmp) {
		/*Lock() - To ensure these operations are protected*/
		pthread_mutex_lock(&dqt->thr_mutex);
		newTimer->timer_remaining = newTimer->timer_interval;		
		dqt->head = newTimer;    
		dqt->timer_elapsed = newTimer->timer_remaining;    
		pthread_mutex_unlock(&dqt->thr_mutex);    

		//printf("Out 1 -> ds_timer_add_to_queue = dqt->head = %d\n",(int)dqt->head);
	}else {       
		/*Lock() - To ensure these operations are protected*/
		pthread_mutex_lock(&dqt->thr_mutex);
		//printf("newTimer Interval = %d\n",newTimer->timer_interval);
		/* Check and Change timer_remaining for head node only */				
		if(dqt->timer_elapsed > 0) {
			tmp->timer_remaining = dqt->timer_elapsed;
		}
		
		while(NULL != tmp) {       
			timer_total += tmp->timer_remaining;

			//printf("timer_total = %d, tmp = %d\n",timer_total,(int)tmp);
			if(timer_total > newTimer->timer_interval) {
				/*Reached the location to insert the newTimer               */
				newTimer->timer_remaining = newTimer->timer_interval - (timer_total - tmp->timer_remaining);         
				//printf("new timer_remaining = %d\n",newTimer->timer_remaining);			
				break;
			}			
			prev = tmp;
			tmp = tmp->next;          
		}

	    /* If last Node reached and timer_interval is larger than all nodes in queue then insert at the end of delta list queue with new Elapsed data*/
		if(NULL == tmp) {
			newTimer->timer_remaining = (newTimer->timer_interval - timer_total);
			//printf("Last node = new timer_remaining = %d\n",newTimer->timer_remaining);
		}
		
	    newTimer->next = tmp;
		if(dqt->head == tmp) { 
			/*
			* Check if change of head node is required if yes then adjust current (head) node remining time, 
	        * push it, change the elpased time and then change the head node
	        */
			tmp->timer_remaining = (tmp->timer_remaining - newTimer->timer_remaining); 
			dqt->timer_elapsed = newTimer->timer_remaining;
			dqt->head = newTimer;
		}else {
	    	prev->next = newTimer;
		}
		
	    pthread_mutex_unlock(&dqt->thr_mutex); 
		//printf("Out Last -> ds_timer_add_to_queue\n");
	}
}

#ifdef _TEST_CODE_
/* Testing - print delta list timers */
void print_timers(void)
{
	struct dq_timer *tmp = dqt->head;
	int i = 0;
	while(NULL != tmp) {
		printf("Slot %d timer is %d, Interval = %d, Delta time (remaining) = %d\n",++i, *(int *)tmp->cb_ctx, tmp->timer_interval, tmp->timer_remaining);
		tmp = tmp->next;
	}
}
#endif

/***********************************************
*
* Below functions are exported library API functions
*
***********************************************/

/*
 * Allocates the memory for new timer data structure
 */
int dq_timer_create(int timer_interval, timer_callback_t cb, void *cb_ctx, void **timer_obj)
{
	/* Allocate a memory to new ds_timer_deltaq data structure.
	   If memory allocation successful set its members & return address
	   Otherwise return NULL. User program to take care of adding it to dq */
	struct dq_timer *newTimer;
	//printf("\nIn -> dq_timer_create\n");

	if (NULL == dqt) {
	    return -ENOENT;
	}

	/* Don't do anything if timerInterval is less than or equal to 0 */
	if (timer_interval <= 0 || NULL == cb || NULL == cb_ctx || NULL == *timer_obj) {
	    return -EINVAL;
	}

	newTimer = (struct dq_timer *) malloc (sizeof(struct dq_timer)); 
	if (NULL == newTimer) {
		return -ENOMEM;
	}

	newTimer->timer_interval = timer_interval;
	newTimer->timer_remaining = 0;
	newTimer->cb = cb;    
	newTimer->cb_ctx = cb_ctx;
	newTimer->next = NULL;
	   
	*timer_obj = (void *)newTimer;
	
	/* Add to queue */
	ds_timer_add_to_queue(newTimer);
	//printf("Out Last -> dq_timer_create\n\n\n");
	return 0;
}

/*
* delete the timer
*/
int dq_timer_delete(struct dq_timer *newTimer)
{
    struct dq_timer *tmp = dqt->head;
	
    if(NULL == newTimer && NULL == dqt->head)
        return -ENOENT;
	//printf("In - delete timer from queue\n");
	
    if(tmp == newTimer) {/*head node*/
        pthread_mutex_lock(&dqt->thr_mutex);
		/* Check and Change timer_remaining value for next node only */
		if((0 < tmp->timer_remaining) && (NULL != tmp->next)) {
			tmp->next->timer_remaining += tmp->timer_remaining;
		}
 
		if(NULL != tmp->next) {
			dqt->timer_elapsed = tmp->next->timer_remaining;
		}
        dqt->head = tmp->next;		
        pthread_mutex_unlock(&dqt->thr_mutex);
        free(tmp); 
		//printf("Out 1 - head - delete timer from queue\n");
        return 0;
    }else {/*Any other node*/
        /*
		* Remove the node if found 
		* Add the timer_remaining value of deleted node to next node's timer_remaining
		*/
        pthread_mutex_lock(&dqt->thr_mutex);
        while(newTimer != tmp->next)
            tmp = tmp->next;
        
        if(NULL != tmp) {
			/* Check and Change timer_remaining value for next node only */
			if(NULL != newTimer->next) {
				newTimer->next->timer_remaining += newTimer->timer_remaining;
			}
            tmp->next = newTimer->next;
            free(newTimer);
            pthread_mutex_unlock(&dqt->thr_mutex);
			//printf("Out 2 - mid node - delete timer from queue\n");
            return 0;
        } else {
            pthread_mutex_unlock(&dqt->thr_mutex);
           	//printf("Out 3 - no head - delete timer from queue\n");
            return -1;
        }
    }  
}

/* Initialize timer */
int dq_timer_init(int timer_resolution) 
{
	int ret = 0;
	dqt = (struct dq_timer_desc *) malloc (sizeof(struct dq_timer_desc));
	if(NULL == dqt) 
		return -ENOMEM;/*-22*/

	dqt->head = NULL;
	dqt->schedule = FALSE;
	dqt->exit = FALSE;	
	dqt->timer_elapsed = 0;
	/* Initialize the thread related mutex and cond variables. */
	pthread_mutex_init(&dqt->thr_mutex, NULL);
	
	/* One time set up */
	sem_init(&timer_sem, /*not shared*/ 0, /*initial value*/0);
	ret = pthread_create(&dqt->thread_id, (pthread_attr_t*)0, timer_schedule_func, (void *)dqt);
	if(0 != ret)
		return ret;

	signal(SIGALRM, timer_signal_handler);
	set_periodic_timer(timer_resolution);/* 1ms => 1000 */
	
	printf("Timer and List Setup Done\n");
	return 0;
}

/* De initialize timers */
void dq_timer_deinit(void) 
{
	timer_stopped = TRUE;
	sem_post(&timer_sem);

	/*Ensure all timer objects are deleted before deleting base timer*/
	pthread_mutex_lock(&dqt->thr_mutex);
	if(NULL != dqt) {
		struct dq_timer *tmp = dqt->head;
		while(NULL != tmp) {
			dqt->head = tmp->next;
			free(tmp);
			tmp = dqt->head;
		}
		free(dqt);	
	}
	pthread_mutex_unlock(&dqt->thr_mutex);

	pthread_mutex_destroy(&dqt->thr_mutex);
	pthread_exit(&dqt->thread_id);
}


