#include "../server/classes/listensock.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

struct addrinfo *create_addrinfo(void)
{
    struct addrinfo hints, *addr = NULL;
    int ret;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if ((ret = getaddrinfo(NULL, "9876", &hints, &addr)) != 0)
    {
        std::cerr << gai_strerror(ret) << std::endl;
        throw std::runtime_error("getaddrinfo broke");
    }

    return addr;
}

/* The listen_socket has a couple of pure-virtual methods, so we need
 * to derive something so we can actually test it.
 */
class test_listen_socket : public listen_socket
{
  public:
    test_listen_socket(struct addrinfo *a) : listen_socket(a) {};
    virtual ~test_listen_socket() {};

    virtual void start(void) {};
    virtual void do_login(uint64_t a, Control *b, access_list& c, int d) {};
};

TEST(BaseUserTest, CreateDelete)
{
    Control *con = new Control(0LL, NULL);
    base_user *base = NULL;

    ASSERT_NO_THROW(
        {
            base = new base_user(0LL, con);
        });
    ASSERT_EQ(base->userid, 0LL);
    ASSERT_EQ(base->control, con);
    ASSERT_EQ(base->pending_logout, false);

    delete base;
    delete con;
}

TEST(BaseUserTest, LessThan)
{
    Control *con1 = new Control(123LL, NULL);
    Control *con2 = new Control(124LL, NULL);
    base_user *base1 = new base_user(123LL, con1);
    base_user *base2 = new base_user(124LL, con2);

    ASSERT_TRUE(*base1 < *base2);

    delete base2;
    delete base1;
    delete con2;
    delete con1;
}

TEST(BaseUserTest, EqualTo)
{
    Control *con1 = new Control(123LL, NULL);
    Control *con2 = new Control(124LL, NULL);
    base_user *base1 = new base_user(123LL, con1);
    base_user *base2 = new base_user(124LL, con2);

    ASSERT_FALSE(*base1 == *base2);

    delete base2;
    delete base1;
    delete con2;
    delete con1;
}

TEST(BaseUserTest, Assignment)
{
    Control *con1 = new Control(123LL, NULL);
    Control *con2 = new Control(124LL, NULL);
    base_user *base1 = new base_user(123LL, con1);
    base_user *base2 = new base_user(124LL, con2);

    ASSERT_FALSE(*base1 == *base2);

    *base1 = *base2;

    ASSERT_TRUE(*base1 == *base2);

    delete base2;
    delete base1;
    delete con2;
    delete con1;
}

TEST(ListenSocketTest, CreateDelete)
{
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;

    ASSERT_NO_THROW(
        {
            listen = new test_listen_socket(addr);
        });
    /* Thread pools should not be started */
    ASSERT_TRUE(listen->send_pool->pool_size() == 0);
    ASSERT_TRUE(listen->access_pool->pool_size() == 0);

    delete listen;
}

TEST(ListenSocketTest, Logout)
{
    Control *control = new Control(123LL, NULL);
    base_user *bu = new base_user(123LL, control);
    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen;
    access_list access;

    listen = new test_listen_socket(addr);
    listen->users[123LL] = bu;

    ASSERT_TRUE(listen->send_pool->queue_size() == 0);

    /* The logout method never looks at the packet (access.buf), so we
     * won't bother setting anything in it.
     */
    access.parent = listen;
    access.what.logout.who = 123LL;

    listen->logout_user(access);

    ASSERT_TRUE(bu->pending_logout == true);
    ASSERT_TRUE(listen->send_pool->queue_size() > 0);

    delete listen;
    delete control;
}
