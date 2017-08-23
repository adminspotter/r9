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
