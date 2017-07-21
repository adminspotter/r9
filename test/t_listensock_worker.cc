#include "../server/classes/listensock.h"
#include "../server/config_data.h"

#include <stdlib.h>
#include <unistd.h>
#include <gtest/gtest.h>

#include "mock_server_globals.h"

int sleep_count, login_count, logout_count;

unsigned int sleep(unsigned int a)
{
    ++sleep_count;
    return a;
}

struct addrinfo *create_addrinfo(void)
{
    struct addrinfo hints, *addr = NULL;
    static int port = 9876;
    char port_str[6];
    int ret;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
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

class test_listen_socket : public listen_socket
{
  public:
    test_listen_socket(struct addrinfo *a) : listen_socket(a) {};
    virtual ~test_listen_socket() {};

    virtual std::string port_type(void) override
        {
            return "test";
        };

    virtual void do_login(uint64_t a, Control *b, access_list& c) override
        {
            delete b;
            ++login_count;
        };
    virtual void do_logout(base_user *a) override
        {
            ++logout_count;
        };
};
