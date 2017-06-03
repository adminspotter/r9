#include "../server/classes/thread_pool.h"

#include <iostream>

#include <gtest/gtest.h>

bool pthread_mutex_init_error = false, pthread_cond_init_error = false;
int mutex_destroy_count, cond_broadcast_count, cond_destroy_count;

int pthread_mutex_init(pthread_mutex_t *a, const pthread_mutexattr_t *b)
{
    if (pthread_mutex_init_error == true)
        return EINVAL;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *a)
{
    ++mutex_destroy_count;
    return 0;
}

int pthread_cond_init(pthread_cond_t *a, const pthread_condattr_t *b)
{
    if (pthread_cond_init_error == true)
        return EINVAL;
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *a)
{
    ++cond_broadcast_count;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *a)
{
    ++cond_destroy_count;
    return 0;
}

void *thread_worker(void *arg)
{
    ThreadPool<int> *pool = (ThreadPool<int> *)arg;
    int what;

    for (;;)
    {
        /* Eat whatever's in the queue */
        pool->pop(&what);
    }
    return NULL;
}

TEST(ThreadPoolTest, CreateDelete)
{
    ThreadPool<int> *pool = NULL;

    ASSERT_NO_THROW(
        {
            pool = new ThreadPool<int>("test", 1);
        });
    ASSERT_TRUE(pool->pool_size() == 0);
    ASSERT_EQ(pool->clean_on_pop, false);
    ASSERT_TRUE(pool->startup_arg == NULL);

    cond_broadcast_count = mutex_destroy_count = cond_destroy_count = 0;
    delete pool;
    ASSERT_EQ(cond_broadcast_count, 1);
    ASSERT_EQ(mutex_destroy_count, 1);
    ASSERT_EQ(cond_destroy_count, 1);
}

TEST(ThreadPoolTest, MutexInitFailure)
{
    ThreadPool<int> *pool = NULL;

    pthread_mutex_init_error = true;
    ASSERT_THROW(
        {
            pool = new ThreadPool<int>("rut-roh", 1);
        },
        std::runtime_error);
    pthread_mutex_init_error = false;
}

TEST(ThreadPoolTest, CondInitFailure)
{
    ThreadPool<int> *pool = NULL;

    pthread_cond_init_error = true;
    ASSERT_THROW(
        {
            pool = new ThreadPool<int>("oh-noes", 1);
        },
        std::runtime_error);
    pthread_cond_init_error = false;
}

TEST(ThreadPoolTest, StartStop)
{
    ThreadPool<int> *pool;
    int pool_size;

    ASSERT_NO_THROW(
        {
            pool = new ThreadPool<int>("go_stop", 1);
        });
    /* Nothing should be started yet */
    pool_size = pool->pool_size();
    ASSERT_EQ(pool_size, 0);

    pool->startup_arg = (void *)pool;
    ASSERT_NO_THROW(
        {
            pool->start(thread_worker);
        });
    pool_size = pool->pool_size();
    ASSERT_EQ(pool_size, 1);

    ASSERT_NO_THROW(
        {
            pool->stop();
        });
    pool_size = pool->pool_size();
    ASSERT_EQ(pool_size, 0);

    ASSERT_NO_THROW(
        {
            delete pool;
        });
}

TEST(ThreadPoolTest, ManyWorkers)
{
    ThreadPool<int> *pool = new ThreadPool<int>("many", 5);
    int pool_size = 0, queue_size = 0, element;

    for (element = 1; element < 4; ++element)
        pool->push(element);
    queue_size = pool->queue_size();
    ASSERT_EQ(queue_size, 3);

    pool->startup_arg = (void *)pool;
    pool->start(thread_worker);
    pool_size = pool->pool_size();
    ASSERT_EQ(pool_size, 5);

    /* A race here, and could fail if the host is heavily loaded. */
    sleep(1);
    queue_size = pool->queue_size();
    ASSERT_EQ(queue_size, 0);

    delete pool;
}

TEST(ThreadPoolTest, Grow)
{
    ThreadPool<int> *pool = new ThreadPool<int>("grow", 5);
    int pool_size = 0;

    pool->startup_arg = (void *)pool;
    pool->start(thread_worker);
    pool_size = pool->pool_size();
    ASSERT_EQ(pool_size, 5);

    sleep(1);
    pool->grow(8);
    pool_size = pool->pool_size();
    ASSERT_EQ(pool_size, 8);

    delete pool;
}
