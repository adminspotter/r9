#include "../server/classes/basesock.h"

#include <stdlib.h>

#include <stdexcept>
#include <typeinfo>

#include <gtest/gtest.h>

bool socket_error = false, socket_zero = false, bind_error = false;
bool am_root = false;
int seteuid_count, setegid_count;

void *test_thread_worker(void *arg)
{
    basesock *base = (basesock *)arg;
    int what;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &what);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &what);
    for (;;)
    {
        pthread_testcancel();
        sleep(1);
    }
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

TEST(BasesockTest, CreateDelete)
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

    socket_zero = true;
    ASSERT_NO_THROW(
        {
            base = new basesock(ai);
        });
    ASSERT_STREQ(base->sa->ntop(), "127.0.0.1");
    ASSERT_EQ(base->sa->port(), 1235);
    ASSERT_GE(base->sock, 0);

    delete(base);
    freeaddrinfo(ai);

    if (getenv("R9_TEST_USE_IPv6") != NULL)
    {
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        ret = getaddrinfo("localhost", "1235", &hints, &ai);
        ASSERT_EQ(ret, 0);

        ASSERT_NO_THROW(
            {
                base = new basesock(ai);
            });
        ASSERT_STREQ(base->sa->ntop(), "::1");
        ASSERT_EQ(base->sa->port(), 1235);
        ASSERT_GE(base->sock, 0);

        delete(base);
        freeaddrinfo(ai);
    }
    socket_zero = false;
}

TEST(BasesockTest, BadSocket)
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

    socket_error = true;
    ASSERT_THROW(
        {
            base = new basesock(ai);
        },
        std::runtime_error);
    socket_error = false;
    freeaddrinfo(ai);
}

TEST(BasesockTest, SocketRoot)
{
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "123", &hints, &ai);
    ASSERT_EQ(ret, 0);

    am_root = false;
    seteuid_count = setegid_count = 0;
    ASSERT_THROW(
        {
            base = new basesock(ai);
        },
        std::runtime_error);
    ASSERT_EQ(seteuid_count, 0);
    ASSERT_EQ(setegid_count, 0);

    am_root = true;
    ASSERT_NO_THROW(
        {
            base = new basesock(ai);
        });
    ASSERT_EQ(seteuid_count, 2);
    ASSERT_EQ(setegid_count, 2);

    delete base;
    freeaddrinfo(ai);
    seteuid_count = setegid_count = 0;
    am_root = false;
}

TEST(BasesockTest, BadListen)
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

    bind_error = true;
    ASSERT_THROW(
        {
            base = new basesock(ai);
        },
        std::runtime_error);

    freeaddrinfo(ai);
    bind_error = false;
}

TEST(BasesockTest, StartStop)
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

    ASSERT_NO_THROW(
        {
            base->stop();
        });

    delete(base);
    freeaddrinfo(ai);

    if (getenv("R9_TEST_USE_IPv6") != NULL)
    {
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;
        ret = getaddrinfo("localhost", "1235", &hints, &ai);
        ASSERT_EQ(ret, 0);

        ASSERT_NO_THROW(
            {
                base = new basesock(ai);
            });
        ASSERT_STREQ(base->sa->ntop(), "::1");
        ASSERT_EQ(base->sa->port(), 1235);
        ASSERT_GE(base->sock, 0);

        base->listen_arg = base;
        ASSERT_NO_THROW(
            {
                base->start(test_thread_worker);
            });

        ASSERT_NO_THROW(
            {
                base->stop();
            });

        delete(base);
        freeaddrinfo(ai);
    }
}
