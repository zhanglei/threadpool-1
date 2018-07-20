#include <unistd.h>
#include <stdio.h>
#include "../src/threadpool.h"

extern struct thread_pool threadpool;

void *
whoami(void *_ __attribute__ ((__unused__))) {
    pthread_detach(pthread_self());
    printf("I am HaHa\n");
    return NULL;
}

int
main(void) {
    threadpool.init(4);

    for (int i = 0; i < 10; i++) {
        threadpool.add(whoami, NULL);
    }

    sleep(1);
    return 0;
}
