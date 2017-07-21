#include "../server/classes/listensock.h"

#include <gtest/gtest.h>

int login_count, logout_count;

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
