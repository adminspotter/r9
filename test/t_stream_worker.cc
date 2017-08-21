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
