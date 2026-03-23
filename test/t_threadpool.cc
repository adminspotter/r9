#include <tap++.h>

using namespace TAP;

#include "../server/classes/thread_pool.h"

#include <stdexcept>

void thread_worker(void *arg) {}

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

    delete pool;
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

    pool->stop();
    is(pool->pool_size(), 0, test + "expected pool size");

    delete pool;
}

void test_grow(void)
{
    std::string test = "grow: ";
    ThreadPool<int> *pool = new ThreadPool<int>("grow", 5);

    pool->startup_arg = (void *)pool;
    pool->start(thread_worker);
    is(pool->pool_size(), 5, test + "expected pool size");

    pool->grow(8);
    is(pool->pool_size(), 8, test + "expected pool size");

    delete pool;
}

typedef struct
{
    int foo;
    char bar;
    double baz;
}
test_type;

void test_push_pop(void)
{
    std::string test = "push/pop: ", st;
    ThreadPool<test_type> *pool = new ThreadPool<test_type>("push-pop", 1);
    test_type req = {1, 'a', 1.234}, buf;
    bool result;

    st = "push: ";

    pool->push(req);
    is(pool->queue_size(), 1, test + st + "expected queue size");

    st = "pop: ";

    pool->clean_on_pop = true;
    result = pool->pop(&buf);
    ok(result, test + st + "expected result");
    is(buf.foo, 1, test + st + "expected test foo");
    is(buf.bar, 'a', test + st + "expected test bar");
    ok(buf.baz == 1.234, test + st + "expected test baz");

    st = "pop null buffer: ";

    result = pool->pop(NULL);
    not_ok(result, test + st + "expected result");

    delete pool;
}

int main(int argc, char **argv)
{
    plan(14);

    test_create_delete();
    test_start_stop();
    test_grow();
    test_push_pop();
    return exit_status();
}
