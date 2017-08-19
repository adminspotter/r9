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

TEST(StreamSocketTest, HandlePacket)
{
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);
    base_user *bu = new base_user(123LL, NULL, sts);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;

    bu->timestamp = 0;

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_ACKPKT;

    sts->handle_packet(p, fd);

    ASSERT_NE(bu->timestamp, 0);

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
