/*
Enables portable context switch (i.e. not based machine specific assembly code)
for creating pre-emptive user threads.

This is based on setjmp.h routines rather than those more straightforward
routines in ucontext.h, since they are unsafe with signals, which we need to
use in order to implement pre-emptive threads.  setjmp() stores the execution
state of where it was called, such that a subsequent call to longjmp() with
that state will take us back as though we have just returned from that setjmp()
call.  So whilst not originally intended for this use, it is possible (with
some imagination) to create user-level context switching with these functions,
without directly using machine code.

The trick with this code is to give each context its own stack, which is not
the normal function of setjmp/longjmp.  To do this the signal handler stack is
modified using a standard POSIX function, then a signal is raised and within
the handler function setjmp is called, thus creating a context with a different
stack to that originally of the process.

Based on the paper, which you should read if you are interested in
understanding the trick more fully:
http://www.gnu.org/software/pth/rse-pmt.ps
Adapted by Nick Blundell 2010
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "context.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define STACK_SIZE 32768 // 32 KB


// Global vars required for context creation.
static context_t    context_caller;  // To remember original context.
static context_t    *context_new;    // The new context we will create.
static void         (*context_create_func)(void *); // Entry function of the new context.
static void         *context_create_arg; // Arg passed to entry function.
static sigset_t     context_create_sigs; // Holds the original set of signals blocked by the process.
                                         // To ensure they are preserved in new context.


void context_create_bootstrap(void);
void context_create_trampoline(int sig);
void unblock_alarm_signals();

// Save the machine context - returns 0 if just set, nonzero if just switched to.
int context_save(context_t *context) {
  // Store the execution context, and our signal mask.
  return sigsetjmp(context->jb,1);
}

/* Restore machine context */
void context_restore(context_t *context) {
  // Restore an execution context and signal mask.
	siglongjmp(context->jb, 1);
}

/* 
Switch from one machine context to another - note that alarm signals are
unblocked after a call to context switch, to support pre-emption.
*/
void context_switch(context_t *old_context, context_t *new_context) {
  // Save the context, and switch to other, unless just returned from switch.
  if (context_save(old_context) == 0) {
    context_restore(new_context); 
  }
  // Ensure pre-emption alarms are unblocked after a context switch.
  unblock_alarm_signals();
}

void context_create(context_t *context, void (*function_addr)(void *), void *function_arg) {
  
  /* step 1 */
  // Block the SIGUSR1 so we do not handle it prematurely, and obtain the original
  // set of blocked signals.
  sigset_t original_signal_set;
  sigset_t signal_set;
  sigemptyset(&signal_set);       
  sigaddset(&signal_set, SIGUSR1);
  sigprocmask(SIG_BLOCK, &signal_set, &original_signal_set); 

  /* step 2 */
  // Setup our trampoline signal handler for manipulating the context stack.
  // We use the SIGUSR1 for this, but any would do.
  struct sigaction signal_action;
  struct sigaction original_signal_action;
  memset((void *)&signal_action, 0, sizeof(struct sigaction));
  signal_action.sa_handler = context_create_trampoline;
  signal_action.sa_flags = SA_ONSTACK; // Use custom stack for handler.
  sigemptyset(&signal_action.sa_mask); // Block no signals in the handler.
  sigaction(SIGUSR1, &signal_action, &original_signal_action);

  /* step 3 */
  // Create an alternate stack for signal handling, which we will exploit for
  // the purpose of creating the new context with its own stack.
  void* stack_addr = malloc(STACK_SIZE);
  struct sigaltstack signal_stack;
  struct sigaltstack original_signal_stack;
  signal_stack.ss_sp = stack_addr;
  signal_stack.ss_size = STACK_SIZE;
  signal_stack.ss_flags = 0;
  sigaltstack(&signal_stack, &original_signal_stack);


  /* step 4 */
  // Set the global variables to describe the context and function, etc. since
  // our signal handler will use these when bootstrapping the context.
  context_new = context;
  context_create_func = function_addr;
  context_create_arg = function_arg;
  context_create_sigs = original_signal_set;
  // Send the signal to ourself, so our handler will be called by kernel (with the custom stack)
  // though note that it will not actually be handled until we unblock it in the next few lines.
  raise(SIGUSR1);  
  
  // Atomically temporarily unblock the signal SIGUSR1 and sleep until the signal handler returns.
  sigfillset(&signal_set);
  sigdelset(&signal_set, SIGUSR1);
  sigsuspend(&signal_set);

  /* step 6 */
  // Now we have our context with its own stack, put things back to how they were.
  sigaltstack(NULL, &signal_stack);
  signal_stack.ss_flags = SS_DISABLE;
  sigaltstack(&signal_stack, NULL); // Disable the custom stack.
  if(!(original_signal_stack.ss_flags & SS_DISABLE))
    sigaltstack(&original_signal_stack, NULL);
  sigaction(SIGUSR1, &original_signal_action, NULL); // Unmap our SIGUSR1 handler. 
  sigprocmask(SIG_SETMASK, &original_signal_set, NULL); // Restore original blocked signal set.

  /* step 7 and step 8 */
  // Now switch into the new context, which will switch back to us immediately prior to calling the
  // the context's entry function (i.e. establishing the function's address and arg on its very own stack.
  context_switch(&context_caller, context);

  /* step 14 */
  return;
}

void context_create_trampoline(int sig) {
  // step 5 */ 
  // This handler is called when our SIGUSR1 signal fires, allowing us to use a
  // custom sigal handler stack.  Whilst we are here we save the execution
  // context, so we can return from the handler normally (shedding handler scope) then
  // slip back in later on to make use of the stack we set up - a beautiful concept.
  if(context_save(context_new)==0) {
    return;                // context with custom stack address.
  }

  // Call a function, to put us into a clean stack frame after the returning
  // then switching back into this handler function.
  context_create_bootstrap();
}



void context_create_bootstrap() {
  /* step 9 */
  // Setup some local pointers to thread function and arg.
  void (*context_start_func)(void *);
  void *context_start_arg;

  /* step 10 */
  // Restore the original signal mask, since handler's block the signal that
  // triggers them by default, and such a bocking mask would have been restored
  // when we switched out of the handler context then back in by siglongjmp.
  sigprocmask(SIG_SETMASK, &context_create_sigs, NULL);

  /* step 11 */
  // Store the address and arg of the thread function locally, since they will not
  // remain forever in their global vars.
  context_start_func = context_create_func;
  context_start_arg = context_create_arg;

  /* step 12 & step 13 */
  // Now everything setup so the next time we are switched to, we will call the function.
  context_switch(context_new, &context_caller);

  // Now unblock alarm signals that may be used for pre-emption, which should have been blocked
  // (and inherited through sigsetjmp) during the creation of this new context.
  unblock_alarm_signals();
  
  /* The thread magically starts... */
  context_start_func(context_start_arg);

  /* Should not be reached */
  fprintf(stderr, "Should not get here - threads should call some kind of thread exit function that causes them to no longer be executed.");
  abort();
}


void unblock_alarm_signals() {
  sigset_t sigset;
  sigset_t original_sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  sigaddset(&sigset, SIGVTALRM); 
  sigprocmask(SIG_UNBLOCK, &sigset, &original_sigset);
}
