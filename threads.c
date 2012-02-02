#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
/********INCLUDE HEADER FILE THREADS.H**********/
#include "threads.h"
/******************/

/************NO NEED TO INCLUDE CONTEXT.H HERE WHEN ALREADY INCLUDED IN THREADS.H FILE */
//#include "context.h"
/************************/

#define TRUE 1
#define FALSE 0
#define READY 0
#define ASLEEP 1
#define TERMINATED 2


/****************/
//NO NEED TO REDEFINE THREAD WHEN IT WAS ALREADY DEFINED IN THREADS.H HEADER FILE
/***************/

/* DEFINE A THREAD */
/* 
typedef struct thread_t {
    context_t *context; 
    int id; 
    int state;
    time_t timestarted;
    int timetosleep; 
} thread_t;
*/

/*DEFINE A DOUBLY LINKED LIST */
typedef struct List {
    struct thread_t *thread; // pointer to the thread  
    struct List *nextThread; // pointer to the next thread in the list
    struct List *prevThread; // pointer to the previous thread in the list
} List;

/*DEFINE POINTERS*/
struct List *head; // pointer to the head of the list
struct List *tail; // pointer to the tail of the list
struct List *current; // pointer to the current running thread in the list

struct context_t maincontext; // stores the main context
struct context_t *running; // pointer to the running context

/*ADDITIONAL VARIABLES*/

int threadcount=0; // number of threads
int idoflastexited;
int firsttail = TRUE;

thread_t* thread_create(void (*function)(void*), void *arg) {

	threadcount++;

	// create a thread with a context

	context_t *context = malloc(sizeof(context_t));
    	struct thread_t *thread = malloc(sizeof (struct thread_t)); // allocate memory for the thread
	context_create(context, function, (void*)arg); // create context	
	thread->context=context;
	thread->id = threadcount;
	thread->state = READY;

		if(head==NULL){ // if the list is empty
			struct List *first = malloc(sizeof(struct List));
		        first->thread = thread;
			first->nextThread = NULL;
	            	first->prevThread = NULL;
 		        head = first;
            		tail = first;

			running = context;	// set this thread to be scheduled first
		}
		else { // if the list is not empty
	            struct List *newt = malloc(sizeof (struct List));
	            newt->prevThread = tail;
	            newt->prevThread->nextThread = newt;
            	    newt->thread = thread;
                    newt->nextThread = head;
                    head->prevThread = newt;
			if(firsttail==TRUE){
				head->nextThread = newt;
				firsttail = FALSE;
			}

                    tail = newt;
		}

	return thread;
}

int thread_id(){
	return current->thread->id; // returns the ID of the current thread
}
void thread_yield(){
	context_switch(running, &maincontext); // switch back 
}

/* REMOVE A THREAD FROM A LIST */
void removethread(){
	
	List *temp = current->nextThread;

	current->prevThread->nextThread = current->nextThread;
	current->nextThread->prevThread = current->prevThread;
	free(current->thread->context);
	free(current->thread);
	//free(current->nextThread);
	//free(current->prevThread);
	free(current);

	current = temp;
}

void thread_exit(){
	idoflastexited = current->thread->id;
	current->thread->state = TERMINATED;
	context_switch(running, &maincontext);
//	removethread();
}
void thread_join(thread_t *p_thread) {

	current = head;
	time_t now;

	while(TRUE)
	{	
		if(idoflastexited==p_thread->id){
			break; // if p_thread has exited, then finish
		}

		if(current->thread->state == ASLEEP){ // if this thread is asleep
					
			time_t before = current->thread->timestarted;
			int timetosleep = current->thread->timetosleep;
			
			time(&now);
		
			// check whether it is time to wake the thread
			if((now-(before+timetosleep))>=0){
				current->thread->state = READY; // awake the thread
			}
		}

		if(current->thread->state == TERMINATED){ // if the thread is terminated then simply do nothing
							  // and skip to the next thread
			
		}

  		struct sigaction action;
  		action.sa_handler = thread_yield;
  		action.sa_flags=0;
  		sigfillset(&action.sa_mask);
		sigaction(SIGALRM, &action, NULL);
		struct itimerval timer_value;
  		timer_value.it_value.tv_sec = 3; // amount of time till a context_switch
  		timer_value.it_value.tv_usec = 0;
 		timer_value.it_interval = timer_value.it_value;
		setitimer(ITIMER_REAL, &timer_value, NULL);

		if(current->thread->state==READY){ // if the thread is ready then perform a context switch
			running = current->thread->context;
			context_switch(&maincontext,running);
		}
		current = current->nextThread; // proceed to the next thread
		
	}
}


void thread_sleep(unsigned int seconds){

	time_t nowt;
	current->thread->timestarted = time(&nowt); // save the current time
	current->thread->state=ASLEEP; // set the thread to a sleepful state
	current->thread->timetosleep=seconds; // save the amount of time to sleep for
	thread_yield();
}
