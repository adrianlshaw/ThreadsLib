/* Example of using your user-threading API. */
#include "stdio.h"
#include "threads.h"

// This thread will eventually exit.
void thread_function_1(void *arg) {
  printf("\nThread %d started with arg %d\n", thread_id(), (int) arg);
  for (int i=0;i<5; i++) {
    printf("%d", thread_id()); fflush(stdout);
    thread_sleep(4);
  }
  thread_exit(); 
}

// This thread will never exit but will yield control to the thread scheduler after
// each iteration.
void thread_function_2(void *arg) {
  printf("\nThread %d started with arg %d\n", thread_id(), (int) arg);
  while(1) {
    printf("%d", thread_id()); fflush(stdout);
    thread_yield();
  }
  thread_exit(); 
}

// This thread will never exit and will not voluntarily yield control to the
// other threads, so your threading API must support pre-emption to restrain
// this.
void thread_function_3(void *arg) {
  printf("\nThread %d started with arg %d\n", thread_id(), (int) arg);
  while(1) {
    printf("%d", thread_id()); fflush(stdout);
    // An inefficient (non-yielding) substitute for a sleep.
    for (int i=0; i<500000000;i++){}; // 500000000
    
  }
  thread_exit(); 
}

void thread_function_4(void *arg) {
 printf("\nThread %d started with arg %d\n", thread_id(), (int) arg);
  while(1) {
    printf("%d", thread_id()); fflush(stdout);
    thread_yield();
  }
  thread_exit();
}

int main() {
  printf("main: started\n");
  
  // Create some threads.

  thread_t *p_thread1 = thread_create(thread_function_1, (void*) 42);

  thread_create(thread_function_2, NULL);

  thread_create(thread_function_3, NULL);

  // Have the main thread wait for one of the threads.
  printf("\nmain: waiting for p_thread1 to finish\n");

  thread_join(p_thread1);

 
  printf("\nmain: Now finishing since thread 1 has finished.\n");
  return 0;
}
