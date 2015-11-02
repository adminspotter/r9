#include "../server/classes/basesock.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <typeinfo>

#include <gtest/gtest.h>

class blowup_basesock : public basesock
{
  public:
    blowup_basesock(struct addrinfo *ai)
        : basesock(ai)
        {
            this->sa = build_sockaddr(*ai->ai_addr);
            this->listen_arg = NULL;
            this->create_socket(ai);
        };
    void create_socket(struct addrinfo *ai)
        {
            std::ostringstream s;
            s << "blammo";
            throw std::runtime_error(s.str());
        };
};

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

TEST(BasesockTest, BadConstructor)
{
    struct addrinfo hints, *ai;
    int ret;
    basesock *base;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1234", &hints, &ai);
    ASSERT_EQ(ret, 0);

    ASSERT_THROW(
        {
            base = new blowup_basesock(ai);
        },
        std::runtime_error);
    freeaddrinfo(ai);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1234", &hints, &ai);
    ASSERT_EQ(ret, 0);

    ASSERT_THROW(
        {
            base = new blowup_basesock(ai);
        },
        std::runtime_error);
    freeaddrinfo(ai);
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

    ASSERT_NO_THROW(
        {
            base = new basesock(ai);
        });
    ASSERT_STREQ(base->sa->ntop(), "127.0.0.1");
    ASSERT_EQ(base->sa->port(), 1235);
    ASSERT_GE(base->sock, 0);

    delete(base);
    freeaddrinfo(ai);

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
