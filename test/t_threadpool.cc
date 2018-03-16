#include <tap++.h>

using namespace TAP;

#include "../server/classes/thread_pool.h"

#include <stdexcept>

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

/* This should never actually be executed, so it'll just sort of sit
 * there.
 */
void *thread_worker(void *arg)
{
    ThreadPool<int> *pool = (ThreadPool<int> *)arg;
    int what;

    for (;;)
    {
        sleep(1);
    }
    return NULL;
}

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    ThreadPool<int> *pool = NULL;

    try
    {
        pool = new ThreadPool<int>("test", 1);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(pool->pool_size(), 0, test + "expected pool size");
    is(pool->clean_on_pop, false, test + "expected clean-on-pop");
    is(pool->startup_arg == NULL, true, test + "expected startup arg");

    cond_broadcast_count = mutex_destroy_count = cond_destroy_count = 0;
    delete pool;
    is(cond_broadcast_count, 1, test + "expected cond broadcasts");
    is(mutex_destroy_count, 1, test + "expected mutex destroys");
    is(cond_destroy_count, 1, test + "expected cond destroys");
}

void test_mutex_init_failure(void)
{
    std::string test = "mutex init failure: ";
    ThreadPool<int> *pool = NULL;

    pthread_mutex_init_error = true;
    try
    {
        pool = new ThreadPool<int>("rut-roh", 1);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("queue mutex"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    pthread_mutex_init_error = false;
}

void test_cond_init_failure(void)
{
    std::string test = "cond init failure: ";
    ThreadPool<int> *pool = NULL;

    pthread_cond_init_error = true;
    try
    {
        pool = new ThreadPool<int>("oh-noes", 1);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("queue not-empty cond"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    pthread_cond_init_error = false;
}

void test_start_stop(void)
{
    std::string test = "start/stop: ";
    ThreadPool<int> *pool;

    try
    {
        pool = new ThreadPool<int>("go_stop", 2);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(pool->pool_size(), 0, test + "expected pool size");

    lock_count = unlock_count = 0;
    pool->startup_arg = (void *)pool;
    try
    {
        pool->start(thread_worker);
    }
    catch (...)
    {
        fail(test + "start exception");
    }
    is(pool->pool_size(), 2, test + "expected pool size");
    is(lock_count > 0, true, test + "performed locks");
    is(lock_count, unlock_count, test + "unlocked all locks");

    join_count = 0;
    pool->stop();
    is(pool->pool_size(), 0, test + "expected pool size");
    is(join_count, 2, test + "expected join count");

    delete pool;
}

void test_start_failure(void)
{
    std::string test = "start failure: ";
    ThreadPool<int> *pool;

    try
    {
        pool = new ThreadPool<int>("kaboom", 1);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(pool->pool_size(), 0, test + "expected pool size");

    pthread_create_error = true;
    lock_count = unlock_count = 0;
    pool->startup_arg = (void *)pool;
    try
    {
        pool->start(thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't start"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    is(pool->pool_size(), 0, test + "expected pool size");
    is(lock_count > 0, true, test + "performed locks");
    is(lock_count, unlock_count, test + "unlocked all locks");

    delete pool;
    pthread_create_error = false;
}

TEST(ThreadPoolTest, Grow)
{
    ThreadPool<int> *pool = new ThreadPool<int>("grow", 5);

    pthread_create_error = false;
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
    ASSERT_GT(lock_count, 0);
    ASSERT_GT(unlock_count, 0);
    ASSERT_EQ(lock_count, unlock_count);
    ASSERT_EQ(signal_count, 1);

    lock_count = unlock_count = 0;
    pool->clean_on_pop = true;
    pool->pop(&buf);
    ASSERT_GT(lock_count, 0);
    ASSERT_GT(unlock_count, 0);
    ASSERT_EQ(lock_count, unlock_count);
    ASSERT_EQ(buf.foo, 1);
    ASSERT_EQ(buf.bar, 'a');
    ASSERT_EQ(buf.baz, 1.234);

    delete pool;
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(19);

    int gtests = RUN_ALL_TESTS();

    test_create_delete();
    test_mutex_init_failure();
    test_cond_init_failure();
    test_start_stop();
    test_start_failure();
    return gtests & exit_status();
}
