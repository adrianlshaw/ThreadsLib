CFLAGS= -Wall -Werror -std=gnu99 -D _GNU_SOURCE

GCC= gcc $(CFLAGS)

test: test.c 
	$(GCC) $< threads.c context.c -o $@

all: 
	make test
