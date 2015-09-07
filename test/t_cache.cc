#include <errno.h>

#include <iostream>

#include "../client/cache.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

bool create_fail = false, cancel_fail = false, join_fail = false;
int each_calls = 0, cleanup_calls = 0;

struct test_struct
{
    int foo, bar;
};

struct test_cleanup
{
    void operator()(test_struct& o) { ++cleanup_calls; }
};

typedef BasicCache<test_struct, test_cleanup> bc_test_t;

void each_function(test_struct& t)
{
    ++each_calls;
}

int pthread_create(pthread_t *a,
                   const pthread_attr_t *b,
                   void *(*c)(void *),
                   void *d)
{
    return (create_fail ? ENOENT : 0);
}

int pthread_cancel(pthread_t a)
{
    return (cancel_fail ? EPERM : 0);
}

int pthread_join(pthread_t a, void **b)
{
    return (join_fail ? EAGAIN : 0);
}

TEST(CacheTest, BasicCreateDelete)
{
    bc_test_t *test_bc = NULL;

    ASSERT_NO_THROW(
        {
            test_bc = new bc_test_t("basic_test");
        });
    ASSERT_TRUE(test_bc != NULL);

    cleanup_calls = 0;
    ASSERT_NO_THROW(
        {
            delete test_bc;
        });
    ASSERT_EQ(cleanup_calls, 0);
}

TEST(CacheTest, BasicElementAccess)
{
    bc_test_t *test_bc = new bc_test_t("access_test");

    /* Insert some elements */

    ASSERT_THROW(
        {
            (*test_bc)[12345];
        },
        std::runtime_error);

    cleanup_calls = 0;
    delete test_bc;
    ASSERT_EQ(cleanup_calls, 0);
}

TEST(CacheTest, BasicElementErase)
{
    bc_test_t *test_bc = new bc_test_t("erase_test");

    /* Insert some elements */

    /* Element that doesn't exist */
    cleanup_calls = 0;
    test_bc->erase(12345);
    ASSERT_EQ(cleanup_calls, 0);

    cleanup_calls = 0;
    delete test_bc;
    ASSERT_EQ(cleanup_calls, 0);
}

TEST(CacheTest, BasicEach)
{
    bc_test_t *test_bc = new bc_test_t("each_test");

    /* Insert some elements */

    each_calls = 0;
    std::function<void(test_struct&)> func = each_function;
    test_bc->each(func);
    ASSERT_EQ(each_calls, 0);

    cleanup_calls = 0;
    delete test_bc;
    ASSERT_EQ(cleanup_calls, 0);
}

TEST(CacheTest, BasicPruneThread)
{
    bc_test_t *test_bc = new bc_test_t("prune_test");

    cleanup_calls = 0;
    delete test_bc;
    ASSERT_EQ(cleanup_calls, 0);
}

TEST(CacheTest, ThreadCreateFailure)
{
    bc_test_t *test_bc = NULL;

    create_fail = true;
    ASSERT_THROW(
        {
            test_bc = new bc_test_t("thread_create_fail");
        },
        std::runtime_error);
    ASSERT_TRUE(test_bc == NULL);
    create_fail = false;
}

TEST(CacheTest, ThreadCancelFailure)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    bc_test_t *test_bc = NULL;

    ASSERT_NO_THROW(
        {
            test_bc = new bc_test_t("thread_cancel_fail");
        });

    /* Insert some objects */

    cancel_fail = true;
    cleanup_calls = 0;
    ASSERT_NO_THROW(
        {
            delete test_bc;
        });
    cancel_fail = false;
    ASSERT_EQ(cleanup_calls, 0);
    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Couldn't cancel thread_cancel_fail reaper thread:"));
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CacheTest, ThreadJoinFailure)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    bc_test_t *test_bc = NULL;

    ASSERT_NO_THROW(
        {
            test_bc = new bc_test_t("thread_join_fail");
        });

    /* Insert some objects */

    join_fail = true;
    cleanup_calls = 0;
    ASSERT_NO_THROW(
        {
            delete test_bc;
        });
    join_fail = false;
    ASSERT_EQ(cleanup_calls, 0);
    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Couldn't reap thread_join_fail reaper thread:"));
    std::clog.rdbuf(old_clog_rdbuf);
}
