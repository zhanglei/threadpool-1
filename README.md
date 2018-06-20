# threadpool
C 语言栈式线程池

## 特性
- 动态扩展、缩减线程池规模

## 示例
> **./example/whoami.c**
      
```
#include <unistd.h>
#include <stdio.h>
#include "../src/threadpool.h"

extern struct thread_pool threadpool;

void *
whoami(void *_ __attribute__ ((__unused__))) {
    printf("I am %zd\n", pthread_self());
    return NULL;
}

int
main(void) {
    // 新建线程池
    // 参数含义：常备线程数：4，动态扩展最大线程数：5(含常备线程数)
    // 第二个参数必须大于第一个参数
    threadpool.init(4, 5);

    for (int i = 0; i < 10; i++) {
        threadpool.add(whoami, NULL);
    }

    sleep(1);
    return 0;
}
```

## 编译 
cd example &&
cc -O2 -lpthread whoami.c ../src/threadpool.c -o whoami     
