# threadpool
C 语言栈式线程池

## 特性
- 动态扩展、缩减线程池规模
- 适用于 fork 产生的子进程

## 示例
> **./example/whoami.c**
      
```
#include <unistd.h>
#include <stdio.h>
#include "../src/threadpool.h"

void *
whoami(void *_ __attribute__ ((__unused__))) {
    printf("I am %zd\n", pthread_self());
    return NULL;
}

int
main(void) {
    // 新建线程池，常备线程数：4
    threadpool.init(4);

    for (int i = 0; i < 10; i++) {
        threadpool.add(whoami, NULL);
    }

    sleep(1);
    return 0;
}
```

## 编译 
```
cd example &&
cc -O2 -lpthread whoami.c ../src/threadpool.c -o whoami     
```
