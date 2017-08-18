#include "../server/classes/listensock.h"
#include "../server/config_data.h"

#include <stdlib.h>
#include <unistd.h>
#include <gtest/gtest.h>

#include "mock_db.h"
#include "mock_server_globals.h"

using ::testing::_;
using ::testing::Return;

int sleep_count;

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
};

TEST(ListenSocketTest, ReaperWorker)
{
    config.send_threads = 1;
    config.access_threads = 1;

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);

    /* This user will be sent a ping */
    base_user *bu = new base_user(123LL, NULL, listen);
    bu->pending_logout = false;
    bu->timestamp = time(NULL) - listen_socket::PING_TIMEOUT - 1;
    listen->users[123LL] = bu;

    /* This user will be logged out entirely */
    GameObject *gob = new GameObject(NULL, NULL, 1234LL);
    base_user *bu2 = new base_user(1234LL, gob, listen);
    bu2->username = "bobbo";
    bu2->pending_logout = true;
    bu2->timestamp = time(NULL) - listen_socket::LINK_DEAD_TIMEOUT - 1;
    listen->users[1234LL] = bu2;

    listen->start();

    /* The user will be deleted absolutely last, so that's what we
     * need to wait for.
     */
    while (listen->users.size() > 1)
        ;

    listen->stop();

    ASSERT_GE(sleep_count, 1);
    ASSERT_TRUE(listen->send_pool->queue_size() > 0);
    ASSERT_TRUE(listen->users.size() == 1);

    delete bu;
    delete gob;
    delete listen;
    freeaddrinfo(addr);
}

TEST(ListenSocketTest, AccessWorker)
{
    config.send_threads = 1;
    config.access_threads = 1;

    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), check_authentication(_, _))
        .WillOnce(Return(0LL));

    struct addrinfo *addr = create_addrinfo();
    listen_socket *listen = new test_listen_socket(addr);

    access_list req;

    memset(&req, 0, sizeof(access_list));
    req.buf.basic.type = TYPE_LOGREQ;
    listen->access_pool->push(req);

    req.buf.basic.type = TYPE_LGTREQ;
    req.what.logout.who = 123LL;
    listen->access_pool->push(req);

    ASSERT_TRUE(listen->access_pool->queue_size() == 2);

    listen->start();

    while (listen->access_pool->queue_size() != 0)
        ;

    listen->stop();

    ASSERT_TRUE(listen->access_pool->queue_size() == 0);

    delete listen;
    freeaddrinfo(addr);
    delete database;
}
