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

TEST(ListenSocketTest, ReaperWorker)
{
    config.send_threads = 1;
    config.access_threads = 1;

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);

    /* This user will be sent a ping */
    Control *control = new Control(123LL, NULL);
    base_user *bu = new base_user(123LL, control);
    bu->pending_logout = false;
    bu->timestamp = time(NULL) - listen_socket::PING_TIMEOUT - 1;
    listen->users[123LL] = bu;

    /* This user will be logged out entirely */
    Control *control2 = new Control(1234LL, NULL);
    base_user *bu2 = new base_user(1234LL, control2);
    bu2->pending_logout = true;
    bu2->timestamp = time(NULL) - listen_socket::LINK_DEAD_TIMEOUT - 1;
    listen->users[1234LL] = bu2;
    GameObject *gob = new GameObject(NULL, NULL, 1234LL);
    control2->slave = gob;

    logout_count = 0;

    listen->start();

    /* Since we have mocked out sleep(), we'll run the system sleep(1)
     * command to give the reaper a chance to do its work.
     */
    system("sleep 1");

    listen->stop();

    ASSERT_GE(sleep_count, 1);
    ASSERT_EQ(logout_count, 1);
    ASSERT_TRUE(listen->send_pool->queue_size() > 0);
    ASSERT_TRUE(listen->users.size() == 1);
    ASSERT_TRUE(gob->natures.find("invisible") != gob->natures.end());
    ASSERT_TRUE(gob->natures.find("non-interactive") != gob->natures.end());

    delete gob;
    delete bu;
    delete control;
    delete listen;
    freeaddrinfo(addr);
}
