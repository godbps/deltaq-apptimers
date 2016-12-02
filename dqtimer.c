/*
 * This code is only for demonstration of software timers with delta list queue
 */

#include "dqtimer_lib.h"


/* Queue list timer call back function code */
void timer_func(void *context) 
{
	/* The code below is just a demonstration. */
	static int i = 1;
	
	printf ("Timer expired at Slot %d, Context = %d\n", i,*(int *)context);
	
	++i;
	if(i == 5) {
		sem_post(&demo_over);
	};
}



/* */
int main()
{
	int timer_resolution = 1000;/* Sets the base timer to trigger periodically at every 1mS */
	int context1, context2, context3, context4; /* variables to store context of timers */
	int ret = -1;
	struct dq_timer ref;
	
	sem_init(&demo_over, /*not shared*/ 0, /*initial value*/0);

	/* Initialize the base timer and the delta list queue */
	dq_timer_init(timer_resolution); 
	
	/*
      1) Creates software timers (1,2,3,4) with different time intervals (in mS) [100,200,50,135]
      2) These timers are sorted during insertion with their timer_remaining variable set to 
      	relative time of expiry of the timer ahead of it in the list
      3) So the values of timer_remaining would show at the beginning (considering no time elapsed) as 
      	[50,50,35,100] -> expiry order of timers = (3,1,4,2).
	  4) A single call back function is used for all timers
	  5) The call back function prints the context values to exhibit the expiry order
	  6) With the last timer expiry the application is closed      
     */
	context1 = 1;
	ret = dq_timer_create(100,timer_func,&context1,(void *)&ref);

	context2 = 2;
	ret = dq_timer_create(200,timer_func,&context2,(void *)&ref);

	context3 = 3;
	ret = dq_timer_create(50,timer_func,&context3,(void *)&ref);

	context4 = 4;
	ret = dq_timer_create(135,timer_func,&context4,(void *)&ref);
	
#ifdef _TEST_CODE_
	print_timers();
#endif
	
	/* Wait for timer_func to be called five times - note that 
	 since we're using signals, sem_wait may fail with EINTR. */
	while (sem_wait(&demo_over) == -1 && errno == EINTR)
	    continue;

	/* De-initialize the delta list queue and base timer */
	dq_timer_deinit();

	return 0;
}
