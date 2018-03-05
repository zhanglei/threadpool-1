#include "threadpool.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int zthread_pool_init(int zSiz, int zGlobSiz);
static int zadd_to_thread_pool(void * (* zFunc) (void *), void *zpParam);

/* 线程池栈结构 */
static int zThreadPollSiz;
static int zOverflowMark;

static zThreadTask__ **zppPoolStack_;

static int zStackHeader;
static pthread_mutex_t zStackHeaderLock;

static pthread_t zThreadPoolTidTrash;

/******************************
 * ====  对外公开的接口  ==== *
 ******************************/
struct zThreadPool__ zThreadPool_ = {
    .init = zthread_pool_init,
    .add = zadd_to_thread_pool,
};

static void *
zthread_pool_meta_func(void *zp_ __attribute__ ((__unused__))) {
    pthread_detach(pthread_self());

    /*
     * 线程池中的线程程默认不接受 cancel
     * 若有需求，在传入的工作函数中更改属性即可
     */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);

    /* 线程任务桩 */
    zThreadTask__ *zpSelfTask = malloc(sizeof(zThreadTask__));
    if (NULL == zpSelfTask) {
        fprintf(stderr, "%s", strerror(errno));
        exit(1);
    }

    zpSelfTask->func = NULL;

    if (0 != pthread_cond_init(&(zpSelfTask->condVar), NULL)) {
        fprintf(stderr, "%s", strerror(errno));
        exit(1);
    }

    if (0 != pthread_mutex_init(&(zpSelfTask->condLock), NULL)) {
        fprintf(stderr, "%s", strerror(errno));
        exit(1);
    }

zMark:
    pthread_mutex_lock(&zStackHeaderLock);

    if (zStackHeader < zOverflowMark) {
        zppPoolStack_[++zStackHeader] = zpSelfTask;
        pthread_mutex_unlock(&zStackHeaderLock);

        pthread_mutex_lock(&zpSelfTask->condLock);
        while (NULL == zpSelfTask->func) {
            /* 等待任务到达 */
            pthread_cond_wait( &(zpSelfTask->condVar), &zpSelfTask->condLock);
        }
        pthread_mutex_unlock(&zpSelfTask->condLock);

        zpSelfTask->func(zpSelfTask->p_param);

        zpSelfTask->func = NULL;
        goto zMark;
    } else {
        pthread_mutex_unlock(&zStackHeaderLock);
    }

    /* 释放占用的系统全局信号量并清理资源占用 */
    sem_post(zThreadPool_.p_threadPoolSem);
    pthread_cond_destroy(&(zpSelfTask->condVar));
    free(zpSelfTask);

    pthread_exit(NULL);
}

/*
 * @param: zGlobSiz 系统全局线程数量上限
 * @param: zSiz 当前线程池初始化成功后会启动的线程数量
 * @return: 成功返回 0，失败返回负数
 */
static int
zthread_pool_init(int zSiz, int zGlobSiz) {
    pthread_mutexattr_t zMutexAttr;

    /*
     * 创建线程池时，先销毁旧锁，再重新初始化之，
     * 防止 fork 之后，在子进程中重建线程池时，形成死锁
     * 锁属性设置为 PTHREAD_MUTEX_NORMAL：不允许同一线程重复取锁
     */
    pthread_mutex_destroy(&zStackHeaderLock);

    pthread_mutexattr_init(&zMutexAttr);
    pthread_mutexattr_settype(&zMutexAttr, PTHREAD_MUTEX_NORMAL);

    pthread_mutex_init(&zStackHeaderLock, &zMutexAttr);

    pthread_mutexattr_destroy(&zMutexAttr);

    /* 允许同时处于空闲状态的线程数量，即常备线程数量 */
    zThreadPollSiz = zSiz;
    zOverflowMark = zThreadPollSiz - 1;

    /*
     * 必须动态初始化为 -1
     * 否则子进程继承父进程的栈索引，将带来异常
     */
    zStackHeader = -1;

    /* 线程池栈结构空间 */
    if (NULL == (zppPoolStack_ = malloc(sizeof(void *) * zThreadPollSiz))) {
        fprintf(stderr, "%s", strerror(errno));
        exit(1);
    }

    /*
     * 主进程程调用时，
     *     zGlobSiz 置为正整数，会尝试清除已存在的旧文件，并新建信号量
     * 子进程中调用时，
     *     zGlobSiz 置为负数或 0，自动继承主进程的 handler
     */
    if (0 < zGlobSiz) {
        sem_unlink("threadpool_sem");
        if (SEM_FAILED ==
                (zThreadPool_.p_threadPoolSem = sem_open("threadpool_sem", O_CREAT|O_RDWR, 0700, zGlobSiz))) {
            fprintf(stderr, "%s", strerror(errno));
            exit(1);
        }
    }

    for (int i = 0; i < zThreadPollSiz; i++) {
        if (0 != sem_trywait(zThreadPool_.p_threadPoolSem)) {
            fprintf(stderr, "%s", strerror(errno));
            exit(1);
        } else {
            if (0 != pthread_create(&zThreadPoolTidTrash, NULL, zthread_pool_meta_func, NULL)) {
                fprintf(stderr, "%s", strerror(errno));
                exit(1);
            }
        }
    }

    return 0;
}


/*
 * 线程池容量不足时，自动扩容
 * 空闲线程过多时，会自动缩容
 * @return 成功返回 0，失败返回 -1
 */
static int
zadd_to_thread_pool(void * (* zFunc) (void *), void *zpParam) {
    pthread_mutex_lock(&zStackHeaderLock);

    while (0 > zStackHeader) {
        pthread_mutex_unlock(&zStackHeaderLock);

        /* 不能超过系统全局范围线程总数限制 */
        sem_wait(zThreadPool_.p_threadPoolSem);

        pthread_create(&zThreadPoolTidTrash, NULL, zthread_pool_meta_func, NULL);

        pthread_mutex_lock(&zStackHeaderLock);
    }

    pthread_mutex_lock(&zppPoolStack_[zStackHeader]->condLock);

    zppPoolStack_[zStackHeader]->func = zFunc;
    zppPoolStack_[zStackHeader]->p_param = zpParam;

    pthread_mutex_unlock(&zppPoolStack_[zStackHeader]->condLock);
    pthread_cond_signal(&(zppPoolStack_[zStackHeader]->condVar));

    zStackHeader--;
    pthread_mutex_unlock(&zStackHeaderLock);

    return 0;
}
