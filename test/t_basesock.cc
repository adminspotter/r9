#include <tap++.h>

using namespace TAP;

#include "../server/classes/basesock.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdexcept>
#include <typeinfo>

#include <gtest/gtest.h>

bool socket_error = false, socket_zero = false, bind_error = false;
bool am_root = false, listen_error = false, pthread_create_error = false;
bool pthread_cancel_error = false, pthread_join_error = false;
bool unlink_error = false;
int seteuid_count, setegid_count;

/* This will never actually be run, so it doesn't need to do anything */
void *test_thread_worker(void *arg)
{
    return NULL;
}

int socket(int a, int b, int c)
{
    if (socket_error == true)
        return -1;
    if (socket_zero == true)
        return 0;
    return 123;
}

int setsockopt(int a, int b, int c, const void *d, socklen_t e)
{
    return 0;
}

int bind(int a, const struct sockaddr *b, socklen_t c)
{
    if (bind_error == true)
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int listen(int a, int b)
{
    if (listen_error == true)
    {
        errno = ENOTSOCK;
        return -1;
    }
    return 0;
}

uid_t getuid(void)
{
    if (am_root == true)
        return 0;
    return 123;
}

uid_t geteuid(void)
{
    return 123;
}

int seteuid(uid_t a)
{
    ++seteuid_count;
    return 0;
}

gid_t getgid(void)
{
    if (am_root == true)
        return 0;
    return 123;
}

gid_t getegid(void)
{
    return 123;
}

int setegid(gid_t a)
{
    ++setegid_count;
    return 0;
}

int pthread_create(pthread_t *a, const pthread_attr_t *b,
                   void *(*c)(void *), void *d)
{
    if (pthread_create_error == true)
        return EINVAL;
    return 0;
}

int pthread_cancel(pthread_t a)
{
    if (pthread_cancel_error == true)
        return EINVAL;
    return 0;
}

int pthread_join(pthread_t a, void **b)
{
    if (pthread_join_error == true)
        return EINVAL;
    return 0;
}

int unlink(const char *a)
{
    if (unlink_error == true)
        return -1;
    return 0;
}

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "v4 addrinfo created successfully");

    socket_zero = true;
    try
    {
        base = new basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(base->sa->ntop(), "127.0.0.1", 10), 0,
       test + "expected address");
    is(base->sa->port(), 1235, test + "expected port");
    ok(base->sock >= 0, test + "socket created");

    delete(base);
    freeaddrinfo(ai);

    if (getenv("R9_TEST_USE_IPv6") != NULL)
    {
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        ret = getaddrinfo("localhost", "1235", &hints, &ai);
        is(ret, 0, test + "v6 addrinfo created successfully");

        try
        {
            base = new basesock(ai);
        }
        catch (...)
        {
            fail(test + "constructor exception");
        }
        is(strncmp(base->sa->ntop(), "::1", 4), 0, test + "expected address");
        is(base->sa->port(), 1235, test + "expected port");
        ok(base->sock >= 0, test + "socket created");

        delete(base);
        freeaddrinfo(ai);
    }
    else
        skip(5, test + "no IPv6");
    socket_zero = false;
}

void test_delete_unix(void)
{
    std::string test = "delete unix: ";
    std::stringstream *new_clog = new std::stringstream;
    auto old_clog_rdbuf = std::clog.rdbuf(new_clog->rdbuf());

    struct addrinfo ai;
    struct sockaddr_un sun;
    basesock *base;

    memset(&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_UNIX;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol = 0;
    ai.ai_addrlen = sizeof(struct sockaddr_un);
    ai.ai_addr = (struct sockaddr *)&sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, "t_basesock.sock", 16);

    try
    {
        base = new basesock(&ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    unlink_error = true;

    delete base;

    unlink_error = false;

    std::clog.rdbuf(old_clog_rdbuf);
    isnt(new_clog->str().find("could not unlink path"), std::string::npos,
         test + "expected unlink failure");

    /* We explicitly leak the new_clog object here because we're
     * seeing segfaults on Linux hosts when we try to delete it.  It's
     * a test, so it's not super-critical that we are leaking
     * something, nor is this object that we're leaking important to
     * the test itself.  We're just trying to make sure that the
     * failure clause was actually executed, and that's what the
     * clause does:  generate an error message in the log.
     */
}

void test_bad_socket(void)
{
    std::string test = "bad socket: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    socket_error = true;
    try
    {
        base = new basesock(ai);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("socket creation failed"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    socket_error = false;
    freeaddrinfo(ai);
}

void test_privileged_socket(void)
{
    std::string test = "privileged socket creation: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "123", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    am_root = false;
    seteuid_count = setegid_count = 0;
    try
    {
        base = new basesock(ai);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("non-root user"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    is(seteuid_count, 0, test + "expected seteuid count");
    is(setegid_count, 0, test + "expected setegid count");

    am_root = true;
    try
    {
        base = new basesock(ai);
    }
    catch (...)
    {
        fail(test + "create_socket exception");
    }
    is(seteuid_count, 2, test + "expected seteuid count");
    is(setegid_count, 2, test + "expected setegid count");

    delete base;
    freeaddrinfo(ai);
    seteuid_count = setegid_count = 0;
    am_root = false;
}

void test_bad_bind(void)
{
    std::string test = "bad bind: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    bind_error = true;
    try
    {
        base = new basesock(ai);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("bind failed for"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    freeaddrinfo(ai);
    bind_error = false;
}

void test_bad_listen(void)
{
    std::string test = "bad listen: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    listen_error = true;
    try
    {
        base = new basesock(ai);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("listen failed for"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    freeaddrinfo(ai);
    listen_error = false;
}

void test_start_stop(void)
{
    std::string test = "start/stop: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "v4 addrinfo created successfully");

    try
    {
        base = new basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(base->sa->ntop(), "127.0.0.1", 10), 0,
       test + "expected address");
    is(base->sa->port(), 1235, test + "expected port");
    ok(base->sock >= 0, test + "socket created");

    base->listen_arg = base;
    try
    {
        base->start(test_thread_worker);
        base->stop();
    }
    catch (...)
    {
        fail(test + "start/stop exception");
    }

    delete(base);
    freeaddrinfo(ai);

    if (getenv("R9_TEST_USE_IPv6") != NULL)
    {
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        ret = getaddrinfo("localhost", "1235", &hints, &ai);
        is(ret, 0, test + "v6 addrinfo created successfully");

        try
        {
            base = new basesock(ai);
        }
        catch (...)
        {
            fail(test + "constructor exception");
        }
        is(strncmp(base->sa->ntop(), "::1", 4), 0, test + "expected address");
        is(base->sa->port(), 1235, test + "expected port");
        ok(base->sock >= 0, test + "socket created");

        base->listen_arg = base;
        try
        {
            base->start(test_thread_worker);
            base->stop();
        }
        catch (...)
        {
            fail(test + "start/stop exception");
        }

        delete(base);
        freeaddrinfo(ai);
    }
    else
        skip(4, test + "no IPv6");
}

void test_start_bad_socket(void)
{
    std::string test = "start w/bad socket: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    socket_zero = true;
    try
    {
        base = new basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    base->listen_arg = base;
    try
    {
        base->start(test_thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("no socket available"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    delete base;
    freeaddrinfo(ai);
    socket_zero = false;
}

void test_start_create_thread_fail(void)
{
    std::string test = "start w/bad pthread_create: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    pthread_create_error = true;
    try
    {
        base = new basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    base->listen_arg = base;
    try
    {
        base->start(test_thread_worker);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't start listen thread"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    delete base;
    freeaddrinfo(ai);
    pthread_create_error = false;
}

void test_stop_cancel_fail(void)
{
    std::string test = "stop w/bad pthread_cancel: ";
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    is(ret, 0, test + "addrinfo created successfully");

    try
    {
        base = new basesock(ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(strncmp(base->sa->ntop(), "127.0.0.1", 10), 0,
       test + "expected address");
    is(base->sa->port(), 1235, test + "expected port");
    ok(base->sock >= 0, test + "socket created");

    base->listen_arg = base;
    try
    {
        base->start(test_thread_worker);
    }
    catch (...)
    {
        fail(test + "start exception");
    }

    pthread_cancel_error = true;
    try
    {
        base->stop();
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't cancel listen thread"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }

    try
    {
        delete(base);
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
    freeaddrinfo(ai);
    pthread_cancel_error = false;
}

TEST(BasesockTest, StopJoinFail)
{
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1235", &hints, &ai);
    ASSERT_EQ(ret, 0);

    ASSERT_NO_THROW(
        {
            base = new basesock(ai);
        });
    ASSERT_STREQ(base->sa->ntop(), "127.0.0.1");
    ASSERT_EQ(base->sa->port(), 1235);
    ASSERT_GE(base->sock, 0);

    base->listen_arg = base;
    ASSERT_NO_THROW(
        {
            base->start(test_thread_worker);
        });

    pthread_join_error = true;
    ASSERT_THROW(
        {
            base->stop();
        },
        std::runtime_error);

    ASSERT_NO_THROW(
        {
            delete(base);
        });
    freeaddrinfo(ai);
    pthread_join_error = false;
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(39);

    int gtests = RUN_ALL_TESTS();

    test_create_delete();
    test_delete_unix();
    test_bad_socket();
    test_privileged_socket();
    test_bad_bind();
    test_bad_listen();
    test_start_stop();
    test_start_bad_socket();
    test_start_create_thread_fail();
    test_stop_cancel_fail();
    return gtests & exit_status();
}
