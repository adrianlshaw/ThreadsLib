#include "context.h"
#include <time.h>
typedef struct thread_t {
    context_t *context; 
    int id; 
    int state;
    time_t timestarted;
    int timetosleep; 
} thread_t;

thread_t* thread_create(void (*function)(void*), void *arg);
int thread_id();
void thread_yield();
void removethread();
void thread_exit();
void thread_join(thread_t *p_thread);
void thread_sleep(unsigned int seconds);
