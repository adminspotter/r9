#include "../server/classes/stream.h"
#include "../server/config_data.h"

#include <gtest/gtest.h>

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

/* Send queue test
 *
 * 3 packet_list elements in the send queue:
 *   - one that won't hton
 *   - one that fails to send
 *   - one that sends correctly
 */
TEST(StreamSocketTest, SendWorker)
{
    config.send_threads = 1;
    config.access_threads = 1;

    struct addrinfo *addr = create_addrinfo();
    test_stream_socket *sts = new test_stream_socket(addr);
    base_user *bu = new base_user(123LL, NULL, sts);
    int fd = 99;

    sts->users[bu->userid] = bu;
    sts->fds[fd] = bu;
    sts->user_fds[bu->userid] = fd;
    sts->max_fd = fd + 1;
    FD_SET(fd, &sts->master_readfs);
    bu->timestamp = time(NULL);

    packet_list pl;

    memset(&pl, 0, sizeof(packet_list));
    pl.buf.basic.type = TYPE_ACKPKT;

    pl.who = bu;
    sts->send_pool->push(pl);
    sts->send_pool->push(pl);
    sts->send_pool->push(pl);

    sts->start();

    while (write_stage < 1)
        ;

    delete sts;
    freeaddrinfo(addr);
}
