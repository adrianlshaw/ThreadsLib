/*
This is an example of how to use the context control functions which you will
use in your thread creation and scheduling libary.
*/
#include <stdio.h>
#include "context.h"

// Allocate some global vars for storing machine context state.
context_t main_context;
context_t context_1;
context_t context_2;


void function_1(void* arg) {
  printf("Started function_1 with arg %d\n", (int) arg);
  context_switch(&context_1, &context_2);
  printf("Ended function_1\n");
  context_switch(&context_1, &context_2);
  while(1); // cannot return from function.
}

void function_2(void* arg) {
  printf("Started function_2\n");
  context_switch(&context_2, &context_1);
  printf("Ended function_2\n");
  context_switch(&context_2, &main_context);
  while(1); // cannot return from function.
}


int main() {
  printf("Started app\n");
  context_create(&context_1, function_1, (void*) 101);
  context_create(&context_2, function_2, NULL);
  context_switch(&main_context, &context_1);
  printf("Ended app\n");
}
