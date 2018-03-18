#include <tap++.h>

using namespace TAP;

#include "../server/classes/socket.h"
#include "../server/classes/dgram.h"
#include "../server/classes/stream.h"

#include <stdexcept>

#include "mock_server_globals.h"

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

void test_unix(void)
{
    std::string test = "unix: ";
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

    try
    {
        listen = socket_create(&addr);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("not supported"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
}

void test_stream(void)
{
    std::string test = "stream: ";
    struct addrinfo *addr = create_addrinfo(SOCK_STREAM);
    listen_socket *listen = NULL;

    try
    {
        listen = socket_create(addr);
    }
    catch (...)
    {
        fail(test + "create exception");
    }

    is(dynamic_cast<stream_socket *>(listen) != NULL, true,
       test + "expected type");

    delete listen;
    freeaddrinfo(addr);
}

void test_dgram(void)
{
    std::string test = "dgram: ";
    struct addrinfo *addr = create_addrinfo(SOCK_DGRAM);
    listen_socket *listen = NULL;

    try
    {
        listen = socket_create(addr);
    }
    catch (...)
    {
        fail(test + "create exception");
    }

    is(dynamic_cast<dgram_socket *>(listen) != NULL, true,
       test + "expected type");

    delete listen;
    freeaddrinfo(addr);
}

int main(int argc, char **argv)
{
    plan(3);

    test_unix();
    test_stream();
    test_dgram();
    return exit_status();
}
