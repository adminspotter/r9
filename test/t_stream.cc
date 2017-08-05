#include "../server/classes/stream.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

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
