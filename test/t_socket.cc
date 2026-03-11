#include <tap++.h>

using namespace TAP;

#include "../server/classes/socket.h"
#include "../server/classes/dgram.h"
#include "../server/classes/stream.h"

#include <stdexcept>

#include "mock_server_globals.h"

void test_unix(void)
{
    std::string test = "unix: ";
    Addrinfo_un *addr = new Addrinfo_un("/no/such/path");

    listen_socket *listen = NULL;

    try
    {
        listen = socket_create(addr);
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
    delete addr;
}

void test_stream(void)
{
    std::string test = "stream: ";
    Addrinfo *addr = new Addrinfo(STREAM, "localhost", "7654");
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
    delete addr;
}

void test_dgram(void)
{
    std::string test = "dgram: ";
    Addrinfo *addr = new Addrinfo(DGRAM, "localhost", "7654");
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
    delete addr;
}

int main(int argc, char **argv)
{
    plan(3);

    test_unix();
    test_stream();
    test_dgram();
    return exit_status();
}
