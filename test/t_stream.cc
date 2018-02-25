#include "../server/classes/stream.h"
#include "../server/classes/config_data.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

bool stop_error = false;
bool select_failure = false, select_eintr = false;
bool linger_valid = false, keepalive_valid = false;
bool read_nothing = false, read_bad_packet = false;

/* In order to test some of the non-public members, we need to coerce
 * them into publicness.
 */
class test_stream_socket : public stream_socket
{
  public:
    using stream_socket::max_fd;
    using stream_socket::readfs;
    using stream_socket::master_readfs;

    test_stream_socket(struct addrinfo *a) : stream_socket(a) {};
    ~test_stream_socket() {};

    void stop(void) override
        {
            if (stop_error == true)
                throw std::runtime_error("oh noes!");
        };
};

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

int accept(int a, struct sockaddr *b, socklen_t *c)
{
    return 99;
}

int setsockopt(int a, int b, int c, const void *d, socklen_t e)
{
    if (c == SO_LINGER)
    {
        struct linger *ls = (struct linger *)d;
        if (ls->l_onoff == (config.use_linger ? 1 : 0)
            && ls->l_linger == config.use_linger)
            linger_valid = true;
    }
    if (c == SO_KEEPALIVE)
        if (*(int *)d == (config.use_keepalive ? 1 : 0))
            keepalive_valid = true;
    return 0;
}

ssize_t read(int a, void *b, size_t c)
{
    if (read_nothing == true)
        return 0;

    ((ack_packet *)b)->type = TYPE_ACKPKT;
    if (read_bad_packet == true)
        return 1;
    return sizeof(ack_packet);
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
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, CreateDeleteStopError)
{
    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts;

    ASSERT_NO_THROW(
        {
            sts = new test_stream_socket(addr);
        });

    stop_error = true;
    ASSERT_NO_THROW(
        {
            delete sts;
        });
    stop_error = false;
    freeaddrinfo(addr);
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

TEST(StreamSocketTest, HandleLogin)
{
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);
    int fd = 99;

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_LOGREQ;

    ASSERT_TRUE(sts->access_pool->queue_size() == 0);

    stream_socket::handle_login(sts, p, NULL, (void *)&fd);

    ASSERT_TRUE(sts->access_pool->queue_size() != 0);
    access_list al;
    memset(&al, 0, sizeof(access_list));
    sts->access_pool->pop(&al);
    ASSERT_EQ(al.buf.basic.type, TYPE_LOGREQ);
    ASSERT_EQ(al.what.login.who.stream, fd);

    delete sts;
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, ConnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);

    ASSERT_TRUE(sts->users.size() == 0);
    ASSERT_TRUE(sts->fds.size() == 0);
    ASSERT_TRUE(sts->user_fds.size() == 0);

    base_user *bu = new base_user(123LL, NULL, sts);

    access_list al;

    memset(&al, 0, sizeof(access_list));
    al.what.login.who.stream = 99;
    al.buf.basic.type = TYPE_LOGREQ;
    strncpy(al.buf.log.username, "bobbo", sizeof(al.buf.log.username));
    strncpy(al.buf.log.password, "argh!", sizeof(al.buf.log.password));
    strncpy(al.buf.log.charname, "howdy", sizeof(al.buf.log.charname));

    sts->connect_user(bu, al);

    ASSERT_TRUE(sts->users.size() == 1);
    ASSERT_TRUE(sts->fds.size() == 1);
    ASSERT_TRUE(sts->user_fds.size() == 1);

    delete sts;
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, DisconnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);
    base_user *bu = new base_user(123LL, NULL, sts);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->master_readfs);

    ASSERT_TRUE(sts->users.size() == 1);
    ASSERT_TRUE(sts->fds.size() == 1);
    ASSERT_TRUE(sts->user_fds.size() == 1);

    sts->disconnect_user(bu);

    ASSERT_TRUE(sts->users.size() == 0);
    ASSERT_TRUE(sts->fds.size() == 0);
    ASSERT_TRUE(sts->user_fds.size() == 0);
    ASSERT_EQ(sts->max_fd, fd);
    ASSERT_TRUE(!FD_ISSET(fd, &sts->master_readfs));

    delete bu;
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

TEST(StreamSocketTest, AcceptNewConnection)
{
    config.use_linger = 123;
    config.use_keepalive = true;

    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);

    FD_SET(sts->sock.sock, &sts->readfs);

    sts->accept_new_connection();

    /* Our fake accept() returns 99 */
    ASSERT_TRUE(sts->fds.find(99) != sts->fds.end());
    ASSERT_TRUE(sts->fds[99] == NULL);
    ASSERT_TRUE(FD_ISSET(99, &sts->master_readfs));
    ASSERT_EQ(sts->max_fd, 100);

    ASSERT_EQ(linger_valid, true);
    ASSERT_EQ(keepalive_valid, true);

    /* Attempts to mock out ioctl have failed, so we won't worry about
     * that setting, and will have no assertion for it.
     */

    delete sts;
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, HandleUsersBadPacket)
{
    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);
    base_user *bu = new base_user(123LL, NULL, sts);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->readfs);

    bu->timestamp = 0;

    read_bad_packet = true;

    sts->handle_users();

    ASSERT_EQ(bu->timestamp, 0);

    read_bad_packet = false;

    delete sts;
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, HandleUsersReadError)
{
    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);
    base_user *bu = new base_user(123LL, NULL, sts);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->readfs);

    read_nothing = true;

    sts->handle_users();

    ASSERT_EQ(bu->pending_logout, true);
    ASSERT_TRUE(!FD_ISSET(fd, &sts->master_readfs));

    read_nothing = false;

    delete sts;
    freeaddrinfo(addr);
}

TEST(StreamSocketTest, HandleUsers)
{
    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);
    base_user *bu = new base_user(123LL, NULL, sts);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->readfs);

    bu->timestamp = 0;

    sts->handle_users();

    ASSERT_NE(bu->timestamp, 0);

    delete sts;
    freeaddrinfo(addr);
}
