#include "../server/classes/thread_pool.h"

#include <iostream>

#include <gtest/gtest.h>

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
    pool->resize(8);
    pool_size = pool->pool_size();
    ASSERT_EQ(pool_size, 8);

    delete pool;
}