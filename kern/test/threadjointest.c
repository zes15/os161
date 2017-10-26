#include <types.h>
#include <lib.h>
#include <test.h>
#include <synch.h>
#include <clock.h>
#include <thread.h>
#include <wchan.h>

#define NTHREADS 12
#define NCVS 250
#define NLOOPS 3

static struct cv * t_join_cv;
static struct lock * t_join_lk;
static struct lock * testlocks[NCVS];
static struct cv * testcvs[NCVS];

static void
initialize_synch_data_structures(void)
{
	// initialize test lock
	t_join_lk = lock_create("threadjointest");
	if(t_join_lk == NULL) {
		panic("failed to create lock\n");
	}
	
	// initialize test cv
	t_join_cv = cv_create("threadjointest");
	if(t_join_cv == NULL) {
		panic("failed to create cv\n");
	}

	// init test locks and cvs
	for(int i = 0; i < NCVS; i++) {
		testlocks[i] = lock_create("threadjoin testlock");
		testcvs[i] = cv_create("threadjoin testcv");
		if(testlocks[i] == NULL || testcvs[i] == NULL) {
			panic("failed to create locks or cvs");
		}
	}
}

static void
destroy_synch_data_structures(void)
{
	lock_destroy(t_join_lk);
	cv_destroy(t_join_cv);
	for(int i = 0; i < NCVS; i++) {
		lock_destroy(testlocks[i]);
		cv_destroy(testcvs[i]);
		testlocks[i] = NULL;
		testcvs[i] = NULL;
	}
}
static void entrypoint_for_my_fork(void * ignore, unsigned long i)
{
	(void)ignore;

	/* put thread to sleep */
	cv_wait(testcvs[i], testlocks[i]);
	
	/* print our thread id */
	kprintf("Forking thread ID %ld\n", i);
}


int threadjointest(int argc, char ** args)
{
	int tid;
	kprintf("Starting thread join test..\n");

	initialize_synch_data_structures();

   for(int j = 0; j < NLOOPS; j ++) {
	
	for(int i = 1; i <= NTHREADS; i++) {
		int error = my_fork("worker", NULL, entrypoint_for_my_fork, \
				    NULL, i);
		if(error) {
			panic("Thread fork failed %s\n", strerror(error));
		}
	}
	
	for(int i = 1; i <=  NTHREADS; i++) {
		
		/* wake up our sleeping threads */
		cv_signal(testcvs[i], testlocks[i]);

		tid = thread_join(); /* BONUS */
		
		/* print our thread id */
		kprintf("Joining thread ID %d\n", tid);

	}
	kprintf("\n");
   }
	destroy_synch_data_structures();
	kprintf("Thread join test done.\n");
	(void)argc;
	(void)args;
	return 0;
}



