#include <string>
#include <stdexcept>

#include "dgram.h"
#include "stream.h"

listen_socket *socket_create(struct addrinfo *ai)
{
    /* We don't actually have a unix-domain listening socket type. */
    if (ai->ai_family == AF_UNIX)
    {
        std::string s = "Unix-domain listening sockets are not supported.";
        throw std::runtime_error(s);
    }

    if (ai->ai_socktype == SOCK_STREAM)
        return new stream_socket(ai);
    else if (ai->ai_socktype == SOCK_DGRAM)
        return new dgram_socket(ai);
}
