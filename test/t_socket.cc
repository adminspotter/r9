#include "../server/classes/listensock.h"
#include "../server/classes/dgram.h"
#include "../server/classes/stream.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

listen_socket *socket_create(struct addrinfo *);

struct addrinfo *create_addrinfo(int type)
{
    struct addrinfo hints, *addr = NULL;
    static int port = 7654;
    char port_str[6];
    int ret;

    hints.ai_family = AF_INET;
    hints.ai_socktype = type;
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

TEST(SocketCreateTest, Unix)
{
    struct sockaddr_un sun;
    struct addrinfo addr;

    memset(&addr, 0, sizeof(struct addrinfo));
    addr.ai_family = AF_UNIX;
    addr.ai_socktype = SOCK_STREAM;
    addr.ai_protocol = 0;
    addr.ai_addrlen = sizeof(struct sockaddr_un);
    addr.ai_addr = (struct sockaddr *)&sun;

    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, "/no/such/path", 14);

    listen_socket *listen = NULL;

    ASSERT_THROW(
        {
            listen = socket_create(&addr);
        },
        std::runtime_error);
}

TEST(SocketCreateTest, Stream)
{
    struct addrinfo *addr = create_addrinfo(SOCK_STREAM);
    listen_socket *listen = NULL;

    ASSERT_NO_THROW(
        {
            listen = socket_create(addr);
        });

    ASSERT_TRUE(dynamic_cast<stream_socket *>(listen) != NULL);

    delete listen;
    freeaddrinfo(addr);
}

TEST(SocketCreateTest, Dgram)
{
    struct addrinfo *addr = create_addrinfo(SOCK_DGRAM);
    listen_socket *listen = NULL;

    ASSERT_NO_THROW(
        {
            listen = socket_create(addr);
        });

    ASSERT_TRUE(dynamic_cast<dgram_socket *>(listen) != NULL);

    delete listen;
    freeaddrinfo(addr);
}
