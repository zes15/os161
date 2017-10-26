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

/* used to test all forked threads successfully joined */
static bool * forked_threads;
static bool * joined_threads;

/* used for synchronization throughout tests */
static struct lock * t_join_lk;
static struct lock * testlocks[NCVS];
static struct cv * testcvs[NCVS];

/* init our data structures */
static void
initialize_synch_data_structures(void)
{
	/* init threadid list for checking successful thread join */
	forked_threads = (bool *)kmalloc(NTHREADS*sizeof(bool));
	joined_threads = (bool *)kmalloc(NTHREADS*sizeof(bool));

	// initialize test lock
	t_join_lk = lock_create("t_join_lk");
	if(t_join_lk == NULL) {
		panic("failed to create lock\n");
	}

	// init additional test locks and cvs
	for(int i = 0; i < NCVS; i++) {
		testlocks[i] = lock_create("threadjoin testlock");
		testcvs[i] = cv_create("threadjoin testcv");
		if(testlocks[i] == NULL || testcvs[i] == NULL) {
			panic("failed to create locks/cvs");
		}
	}
}

/* destroy our data structs when we are done */
static void
destroy_synch_data_structures(void)
{
	/* free our thread id list */
	kfree(forked_threads);
	forked_threads = NULL;
	kfree(joined_threads);
	joined_threads = NULL;

	/* free our data struct's memory */
	lock_destroy(t_join_lk);
	for(int i = 0; i < NCVS; i++) {
		lock_destroy(testlocks[i]);
		cv_destroy(testcvs[i]);
		testlocks[i] = NULL;
		testcvs[i] = NULL;
	}
}

/* this func gets called for each forked thread */
static void entrypoint_for_my_fork(void * ignore, unsigned long i)
{
	(void)ignore;

	/* put thread to sleep */
	cv_wait(testcvs[i], testlocks[i]);
	
	/* print our thread id */
	kprintf("Forking thread ID %ld\n", i);
	thread_exit();
}

/* run our tests */
int threadjointest(int argc, char ** args)
{
	int tid;
	kprintf("Starting thread join test..\n");

	initialize_synch_data_structures();

   /* run test nloop times */
   for(int j = 0; j < NLOOPS; j ++) {
	
	for(int i = 1; i <= NTHREADS; i++) {

		/* acquire a lock so each fork comes after one another */
		lock_acquire(t_join_lk);

		/* fork a child thread */
		int error = my_fork("worker", NULL, entrypoint_for_my_fork, \
				    NULL, i);
		/* panic if forking error */
		if(error)
			panic("Thread fork failed %s\n", strerror(error));

		/* add thread id to forked_thread list */
		forked_threads[i] = true;

		/* done with critical fork section */
		lock_release(t_join_lk);
	}
	
	for(int i = 1; i <=  NTHREADS; i++) {
		
		/* wake up our sleeping threads */
		cv_signal(testcvs[i], testlocks[i]);

		/* join our child thread back w/ parent *
		 * and retrieve its thread id so we can *
		 * print it	     *  BONUS	*	*/
		tid = thread_join();

		/* add thread id to joined_thread list */
		joined_threads[i] = true;
		
		/* print our thread id */
		kprintf("Joining thread ID %d\n", tid);

	}

	/* check if we joined all our forked threads */
	for(int k = 1; k <= NTHREADS; k++) {
		if(!forked_threads[k]) {
			panic("Thread %d did not fork.\n", k);
		}

		/* reset forked thread id on list */
		forked_threads[k] = false;

		if(!joined_threads[k]) {
			panic("Thread %d did not join.\n", k);
		}

		/* reset joined thread id on list */
		joined_threads[k] = false;
	}

	/* this test ensures we use synchronization properly *
         * throughout forking and joining, and also ensures  *
         * that all thread ids previously forked are joined  *
         * successfully with thread_join                     */
	kprintf("All forked threads have been successfully joined.\n\n"); 
   }
	destroy_synch_data_structures();
	kprintf("Thread join test done.\n");
	(void)argc;
	(void)args;
	return 0;
}



