#ifndef THREADPOOL_H
#define THREADPOOL_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <pthread.h>

struct thread_task {
    pthread_cond_t cond_var;
    char __pad[128];
    pthread_mutex_t cond_lock;

    void * (* fn) (void *);
    void *p_param;
};

struct thread_pool {
    int (* init) (int);
    int (* add) (void * (*) (void *), void *);
};

struct thread_pool threadpool;

#endif
