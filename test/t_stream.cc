#include <tap++.h>

using namespace TAP;

#include "../server/classes/stream.h"
#include "../server/classes/config_data.h"

#include "mock_base_user.h"
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

    test_stream_socket() : stream_socket() {};
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

void test_create_delete(void)
{
    std::string test = "create/delete: ";
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts;

    try
    {
        sts = new stream_socket(addr);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    delete sts;
    freeaddrinfo(addr);
}

void test_create_delete_stop_error(void)
{
    std::string test = "create/delete w/stop error:";
    test_stream_socket *sts;

    try
    {
        sts = new test_stream_socket();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    stop_error = true;
    try
    {
        delete sts;
    }
    catch (...)
    {
        fail(test + "destructor exception");
    }
    stop_error = false;
}

void test_port_type(void)
{
    std::string test = "port type: ";
    test_stream_socket *sts = new test_stream_socket();

    is(sts->port_type(), "stream", test + "expected port type");

    delete sts;
}

void test_start_stop(void)
{
    std::string test = "start/stop: ";
    config.send_threads = 1;
    config.access_threads = 1;

    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);
    freeaddrinfo(addr);

    try
    {
        sts->start();
    }
    catch (...)
    {
        fail(test + "start exception");
    }

    std::cerr << "this I/O gives threads a chance to start"
              << " and helps prevent hangs" << std::endl;

    try
    {
        sts->stop();
    }
    catch (...)
    {
        fail(test + "stop exception");
    }

    delete sts;
}

void test_handle_packet(void)
{
    std::string test = "handle_packet: ";
    test_stream_socket *sts = new test_stream_socket();
    fake_base_user *bu = new fake_base_user(123LL);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;

    bu->parent = sts;
    bu->timestamp = 0;

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_ACKPKT;

    sts->handle_packet(p, sizeof(ack_packet), fd);

    isnt(bu->timestamp, 0, test + "expected timestamp update");

    delete sts;
}

void test_handle_login(void)
{
    std::string test = "handle_login: ";
    test_stream_socket *sts = new test_stream_socket();
    int fd = 99;

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_LOGREQ;

    is(sts->access_pool->queue_size(), 0,
       test + "expected access queue size");

    stream_socket::handle_login(sts, p, NULL, (void *)&fd);

    isnt(sts->access_pool->queue_size(), 0,
         test + "expected access queue size");
    access_list al;
    memset(&al, 0, sizeof(access_list));
    sts->access_pool->pop(&al);
    is(al.buf.basic.type, TYPE_LOGREQ, test + "expected packet type");
    is(al.what.login.who.stream, fd, test + "expected file descriptor");

    delete sts;
}

void test_connect_user(void)
{
    std::string test = "connect_user: ";
    test_stream_socket *sts = new test_stream_socket();

    is(sts->users.size(), 0, test + "expected user list size");
    is(sts->fds.size(), 0, test + "expected fds size");
    is(sts->user_fds.size(), 0, test + "expected user fds size");

    fake_base_user *bu = new fake_base_user(123LL);
    bu->parent = sts;

    access_list al;

    memset(&al, 0, sizeof(access_list));
    al.what.login.who.stream = 99;
    al.buf.basic.type = TYPE_LOGREQ;
    strncpy(al.buf.log.username, "bobbo", sizeof(al.buf.log.username));
    strncpy(al.buf.log.password, "argh!", sizeof(al.buf.log.password));
    strncpy(al.buf.log.charname, "howdy", sizeof(al.buf.log.charname));

    sts->connect_user(bu, al);

    is(sts->users.size(), 1, test + "expected user list size");
    is(sts->fds.size(), 1, test + "expected fds size");
    is(sts->user_fds.size(), 1, test + "expected user fds size");

    delete sts;
}

void test_disconnect_user(void)
{
    std::string test = "disconnect_user: ";
    test_stream_socket *sts = new test_stream_socket();
    fake_base_user *bu = new fake_base_user(123LL);
    int fd = 99;

    bu->parent = sts;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->master_readfs);

    is(sts->users.size(), 1, test + "expected user list size");
    is(sts->fds.size(), 1, test + "expected fds size");
    is(sts->user_fds.size(), 1, test + "expected user fds size");

    sts->disconnect_user(bu);

    is(sts->users.size(), 0, test + "expected user list size");
    is(sts->fds.size(), 0, test + "expected fds size");
    is(sts->user_fds.size(), 0, test + "expected user fds size");
    is(sts->max_fd, fd, test + "expected max fd");
    is(!FD_ISSET(fd, &sts->master_readfs), true,
       test + "fd not in select set");

    delete bu;
    delete sts;
}

void test_select_fd_set(void)
{
    std::string test = "select_fd_set: ", st;
    int retval;
    struct addrinfo *addr = create_addrinfo();
    stream_socket *sts = new stream_socket(addr);
    freeaddrinfo(addr);

    st = "select EINTR failure: ";

    select_failure = true;
    select_eintr = true;
    retval = sts->select_fd_set();

    is(retval, -1, test + st + "expected return value");
    is(errno, EINTR, test + st + "expected errno");

    st = "other select failure: ";

    select_eintr = false;
    retval = sts->select_fd_set();

    is(retval, -1, test + st + "expected return value");
    is(errno, EINVAL, test + st + "expected errno");

    st = "success: ";

    select_failure = false;
    retval = sts->select_fd_set();

    is(retval, 0, test + st + "expected return value");

    delete sts;
}

void test_accept_new_connection(void)
{
    std::string test = "accept_new_connection: ";
    config.use_linger = 123;
    config.use_keepalive = true;

    test_stream_socket *sts = new test_stream_socket();

    FD_SET(sts->sock.sock, &sts->readfs);

    sts->accept_new_connection();

    /* Our fake accept() returns 99 */
    isnt(sts->fds.find(99), sts->fds.end(), test + "found descriptor");
    is(sts->fds[99] == NULL, true, test + "descriptor is not null");
    isnt(FD_ISSET(99, &sts->master_readfs), 0,
         test + "descriptor in select set");
    is(sts->max_fd, 100, test + "expected max descriptor");

    is(linger_valid, true, test + "expected linger setting");
    is(keepalive_valid, true, test + "expected keeaplive setting");

    /* Attempts to mock out ioctl have failed, so we won't worry about
     * that setting, and will have no assertion for it.
     */

    delete sts;
}

void test_handle_users_bad_packet(void)
{
    std::string test = "handle_users w/bad packet: ";
    test_stream_socket *sts = new test_stream_socket();
    fake_base_user *bu = new fake_base_user(123LL);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->readfs);

    bu->parent = sts;
    bu->timestamp = 0;

    read_bad_packet = true;

    sts->handle_users();

    is(bu->timestamp, 0, test + "no timestamp update");

    read_bad_packet = false;

    delete sts;
}

void test_handle_users_read_error(void)
{
    std::string test = "handle_users w/read error: ";
    test_stream_socket *sts = new test_stream_socket();
    fake_base_user *bu = new fake_base_user(123LL);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->readfs);

    bu->parent = sts;

    read_nothing = true;

    sts->handle_users();

    is(bu->pending_logout, true, test + "user is logging out");
    is(!FD_ISSET(fd, &sts->master_readfs), true,
       test + "descriptor not in select set");

    read_nothing = false;

    delete sts;
}

void test_handle_users(void)
{
    std::string test = "handle_users: ";
    test_stream_socket *sts = new test_stream_socket();
    fake_base_user *bu = new fake_base_user(123LL);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->readfs);

    bu->parent = sts;
    bu->timestamp = 0;

    sts->handle_users();

    isnt(bu->timestamp, 0, test + "expected timestamp update");

    delete sts;
}

int main(int argc, char **argv)
{
    plan(35);

    test_create_delete();
    test_create_delete_stop_error();
    test_port_type();
    test_start_stop();
    test_handle_packet();
    test_handle_login();
    test_connect_user();
    test_disconnect_user();
    test_select_fd_set();
    test_accept_new_connection();
    test_handle_users_bad_packet();
    test_handle_users_read_error();
    test_handle_users();
    return exit_status();
}
