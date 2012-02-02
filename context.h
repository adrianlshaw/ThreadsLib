#include <setjmp.h>

/* Machine context data structure, which simply wraps jmp_buf of setjmp/longjmp */
typedef struct context_t {
	jmp_buf jb;
} context_t;

// Main API
void context_create(context_t *context,void (*function_addr)(void *), void *function_arg); 
int context_save(context_t *context);
void context_restore(context_t *context);
void context_switch(context_t *old_context, context_t *new_context);
