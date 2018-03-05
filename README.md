# threadpool
C 语言栈式线程池

## 调用示例
cc -O2 -lpthread threadpool.c whoami.c -o whoami     

>whoami.c
```
#include <unistd.h>
#include <stdio.h>

#include "threadpool.h"

extern struct zThreadPool__ zThreadPool_;

void *
whoami(void *_ __attribute__ ((__unused__))) {
    printf("I am %zd\n", pthread_self());
    return NULL;
}

int
main(void) {
    zThreadPool_.init(4, 5);

    for (int i = 0; i < 10; i++) {
        zThreadPool_.add(whoami, NULL);
    }

    sleep(1);

    return 0;
}
```