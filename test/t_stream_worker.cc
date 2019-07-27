#include <tap++.h>

using namespace TAP;

#include "../server/classes/stream.h"
#include "../server/classes/config_data.h"

#include "mock_base_user.h"
#include "mock_server_globals.h"

int write_stage = 0;

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
    FD_ZERO(b);
    FD_SET(99, b);
    return 1;
}

ssize_t read(int a, void *b, size_t c)
{
    std::cerr << "fake read" << std::endl;
    ((ack_packet *)b)->type = TYPE_ACKPKT;
    return sizeof(ack_packet);
}

ssize_t write(int a, const void *b, size_t c)
{
    std::cerr << "fake write, stage " << write_stage << std::endl;
    if (write_stage++ == 0)
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

void pthread_testcancel(void)
{
}

int hton_packet(packet *p, size_t s)
{
    static int stage = 0;

    if (stage++ == 0)
        return 0;
    return 1;
}

void test_listen_worker(void)
{
    std::string test = "listen worker: ";
    config.send_threads = 1;
    config.access_threads = 1;

    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);
    fake_base_user *bu = new fake_base_user(123LL);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->master_readfs);

    bu->parent = sts;
    time_t now = time(NULL) - 2;
    bu->timestamp = now;

    main_loop_exit_flag = 0;

    sts->start();

    while (bu->timestamp == now)
        ;

    main_loop_exit_flag = 1;

    delete sts;
    freeaddrinfo(addr);
}

/* Send queue test
 *
 * 3 packet_list elements in the send queue:
 *   - one that won't hton
 *   - one that fails to send
 *   - one that sends correctly
 */
void test_send_worker(void)
{
    std::string test = "send worker: ";
    config.send_threads = 1;
    config.access_threads = 1;

    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);
    fake_base_user *bu = new fake_base_user(123LL);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->master_readfs);

    bu->parent = sts;
    bu->timestamp = time(NULL);

    packet_list pl;

    memset(&pl, 0, sizeof(packet_list));
    pl.buf.basic.type = TYPE_ACKPKT;

    pl.who = bu;
    sts->send_pool->push(pl);
    sts->send_pool->push(pl);
    sts->send_pool->push(pl);

    main_loop_exit_flag = 1;

    sts->start();

    while (write_stage < 2)
        ;

    delete sts;
    freeaddrinfo(addr);
}

int main(int argc, char **argv)
{
    plan(0);

    test_listen_worker();
    test_send_worker();
    return exit_status();
}
