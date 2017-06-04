#include "../server/classes/thread_pool.h"

#include <iostream>

#include <gtest/gtest.h>

bool pthread_mutex_init_error = false, pthread_cond_init_error = false;
bool pthread_create_error = false;
int mutex_destroy_count, cond_broadcast_count, cond_destroy_count;
int create_count, join_count, lock_count, unlock_count, signal_count;

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

int pthread_cond_signal(pthread_cond_t *a)
{
    ++signal_count;
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

int pthread_create(pthread_t *a, const pthread_attr_t *b,
                   void *(*c)(void *), void *d)
{
    ++create_count;
    if (pthread_create_error == true)
        return EINVAL;
    return 0;
}

int pthread_join(pthread_t a, void **b)
{
    ++join_count;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *a)
{
    ++lock_count;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *a)
{
    ++unlock_count;
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

    ASSERT_NO_THROW(
        {
            pool = new ThreadPool<int>("go_stop", 2);
        });
    ASSERT_TRUE(pool->pool_size() == 0);

    lock_count = unlock_count = 0;
    pool->startup_arg = (void *)pool;
    ASSERT_NO_THROW(
        {
            pool->start(thread_worker);
        });
    ASSERT_TRUE(pool->pool_size() == 2);
    ASSERT_EQ(lock_count, 1);
    ASSERT_EQ(unlock_count, 1);

    join_count = 0;
    pool->stop();
    ASSERT_TRUE(pool->pool_size() == 0);
    ASSERT_EQ(join_count, 2);

    delete pool;
}

TEST(ThreadPoolTest, StartError)
{
    ThreadPool<int> *pool;

    ASSERT_NO_THROW(
        {
            pool = new ThreadPool<int>("go_stop", 2);
        });
    ASSERT_TRUE(pool->pool_size() == 0);

    pthread_create_error = true;
    lock_count = unlock_count = 0;
    pool->startup_arg = (void *)pool;
    ASSERT_THROW(
        {
            pool->start(thread_worker);
        },
        std::runtime_error);
    ASSERT_TRUE(pool->pool_size() == 0);
    ASSERT_EQ(lock_count, 1);
    ASSERT_EQ(unlock_count, 1);

    delete pool;
    pthread_create_error = false;
}

TEST(ThreadPoolTest, Grow)
{
    ThreadPool<int> *pool = new ThreadPool<int>("grow", 5);

    pool->startup_arg = (void *)pool;
    pool->start(thread_worker);
    ASSERT_TRUE(pool->pool_size() == 5);

    pool->grow(8);
    ASSERT_TRUE(pool->pool_size() == 8);

    delete pool;
}

typedef struct
{
    int foo;
    char bar;
    double baz;
}
test_type;

TEST(ThreadPoolTest, PushPop)
{
    ThreadPool<test_type> *pool = new ThreadPool<test_type>("push-pop", 1);
    test_type req = {1, 'a', 1.234}, buf;

    lock_count = signal_count = unlock_count = 0;
    pool->push(req);
    ASSERT_EQ(lock_count, 1);
    ASSERT_EQ(signal_count, 1);
    ASSERT_EQ(unlock_count, 1);

    lock_count = unlock_count = 0;
    pool->clean_on_pop = true;
    pool->pop(&buf);
    ASSERT_EQ(lock_count, 1);
    ASSERT_EQ(unlock_count, 1);
    ASSERT_EQ(buf.foo, 1);
    ASSERT_EQ(buf.bar, 'a');
    ASSERT_EQ(buf.baz, 1.234);

    delete pool;
}
