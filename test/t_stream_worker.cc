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
    using stream_socket::users;
    using stream_socket::send_pool;
    using stream_socket::max_fd;
    using stream_socket::readfs;
    using stream_socket::master_readfs;

    test_stream_socket(Addrinfo *a) : stream_socket(a) {};
    ~test_stream_socket() {};
};

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

    Addrinfo *addr = new Addrinfo(STREAM, "localhost", "8765");
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

    sts->start();

    while (bu->timestamp == now)
        ;

    delete sts;
    delete addr;
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

    Addrinfo *addr = new Addrinfo(STREAM, "localhost", "8765");
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

    sts->start();

    while (write_stage < 2)
        ;

    delete sts;
    delete addr;
}

int main(int argc, char **argv)
{
    plan(0);

    test_listen_worker();
    test_send_worker();
    return exit_status();
}
