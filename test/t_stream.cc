#include "../server/classes/stream.h"
#include "../server/config_data.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

bool select_failure = false, select_eintr = false;

struct addrinfo *create_addrinfo(void)
{
    struct addrinfo hints, *addr = NULL;
    static int port = 8765;
    char port_str[6];
    int ret;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    snprintf(port_str, sizeof(port_str), "%d", port--);
    if ((ret = getaddrinfo(NULL, port_str, &hints, &addr)) != 0)
    {
        std::cerr << gai_strerror(ret) << std::endl;
        throw std::runtime_error("getaddrinfo broke");
    }

    return addr;
}

int select(int a, fd_set *b, fd_set *c, fd_set *d, struct timeval *e)
{
    if (select_failure == true)
    {
        if (select_eintr == true)
            errno = EINTR;
        else
            errno = EINVAL;
        return -1;
    }
    return 0;
}

TEST(StreamUserTest, CreateDelete)
{
    stream_user *stu;

    ASSERT_NO_THROW(
        {
            stu = new stream_user(123LL, NULL, NULL);
        });
    ASSERT_EQ(stu->userid, 123LL);
    ASSERT_EQ(stu->pending_logout, false);
    ASSERT_NE(stu->timestamp, 0);
    ASSERT_TRUE(stu->control == NULL);

    delete stu;
}

TEST(StreamUserTest, Assignment)
{
    stream_user *stu1 = new stream_user(123LL, NULL, NULL);
    stream_user *stu2 = new stream_user(987LL, NULL, NULL);

    stu1->fd = 42;

    stu2->fd = 99;

    *stu2 = *stu1;

    ASSERT_EQ(stu1->userid, stu2->userid);
    ASSERT_EQ(stu1->fd, stu2->fd);

    delete stu2;
    delete stu1;
}

TEST(StreamSocketTest, CreateDelete)
{
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts;

    ASSERT_NO_THROW(
        {
            sts = new stream_socket(addr);
        });

    delete sts;
}

TEST(StreamSocketTest, PortType)
{
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);

    ASSERT_TRUE(sts->port_type() == "stream");

    delete sts;
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, StartStop)
{
    config.send_threads = 1;
    config.access_threads = 1;

    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);

    ASSERT_NO_THROW(
        {
            sts->start();
        });

    ASSERT_NO_THROW(
        {
            sts->stop();
        });

    delete sts;
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, SelectFdSet)
{
    int retval;
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);

    select_failure = true;
    select_eintr = true;
    retval = sts->select_fd_set();

    ASSERT_EQ(retval, -1);
    ASSERT_EQ(errno, EINTR);

    select_eintr = false;
    retval = sts->select_fd_set();

    ASSERT_EQ(retval, -1);
    ASSERT_EQ(errno, EINVAL);

    select_failure = false;
    retval = sts->select_fd_set();

    ASSERT_EQ(retval, 0);

    delete sts;
    freeaddrinfo(addr);
}