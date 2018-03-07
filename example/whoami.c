#include <unistd.h>
#include <stdio.h>
#include "../src/threadpool.h"

extern struct thread_pool pool;

void *
whoami(void *_ __attribute__ ((__unused__))) {
    printf("I am %zd\n", pthread_self());
    return NULL;
}

int
main(void) {
    pool.init(4, 5);

    for (int i = 0; i < 10; i++) {
        pool.add(whoami, NULL);
    }

    sleep(1);
    return 0;
}
