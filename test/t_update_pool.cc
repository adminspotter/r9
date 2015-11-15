#include "../server/classes/update_pool.h"
#include "../server/classes/listensock.h"
#include "../server/config_data.h"
#include "../server/classes/modules/db.h"

#include <iostream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

//#include "tap.h"

using ::testing::_;

std::vector<listen_socket *> sockets;

class mock_DB : public DB
{
  public:
    mock_DB(const std::string& a, const std::string& b,
            const std::string& c, const std::string& d)
        : DB::DB(a, b, c, d)
        {
        };
    ~mock_DB() {};

    MOCK_METHOD2(check_authentication, uint64_t(const std::string& a,
                                                const std::string& b));
    MOCK_METHOD2(check_authorization, int(uint64_t a, uint64_t b));
    MOCK_METHOD2(open_new_login, int(uint64_t a, uint64_t b));
    MOCK_METHOD2(check_open_login, int(uint64_t a, uint64_t b));
    MOCK_METHOD2(close_open_login, int(uint64_t a, uint64_t b));
    MOCK_METHOD3(get_player_server_skills,
                 int(uint64_t a, uint64_t b,
                     std::map<uint16_t, action_level>& c));

    /* Server functions */
    MOCK_METHOD1(get_server_skills, int(std::map<uint16_t, action_rec>& a));
    MOCK_METHOD1(get_server_objects, int(std::map<uint64_t, GameObject *>& a));
};

class mock_listen : public listen_socket
{
  public:
    mock_listen(struct addrinfo *a)
        : listen_socket(a)
        {
        };
    ~mock_listen() {};

    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());

    MOCK_METHOD1(login_user, void(access_list& a));
    MOCK_METHOD1(logout_user, void(access_list& a));

    MOCK_METHOD3(do_login, void(uint64_t a, Control *b, access_list& c));
};

class mock_SendPool : public ThreadPool<packet_list>
{
  public:
    mock_SendPool(const char *a, int b)
        : ThreadPool<packet_list>(a, b)
        {
        };
    ~mock_SendPool() {};

    MOCK_METHOD1(start, void(void *(*func)(void *)));
    MOCK_METHOD1(push, void(packet_list& p));
};

DB *database;

TEST(UpdatePoolTest, Operate)
{
    struct addrinfo hints, *ai;
    int ret;
    listen_socket *sock;
    Control *con = NULL;
    UpdatePool *pool;
    mock_SendPool *send_pool = new mock_SendPool("t_send", 1);
    GameObject *go = new GameObject(NULL, NULL, 12345LL);

    EXPECT_CALL(*send_pool, push(_)).Times(3);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    ret = getaddrinfo("localhost", "1238", &hints, &ai);
    ASSERT_EQ(ret, 0);

    ASSERT_NO_THROW(
        {
            sock = new mock_listen(ai);
        });
    delete sock->send_pool;
    sock->send_pool = send_pool;
    sockets.push_back(sock);

    sock->users[1LL] = new base_user(1LL, con);
    sock->users[2LL] = new base_user(2LL, con);
    sock->users[3LL] = new base_user(3LL, con);
    ASSERT_EQ(sock->users.size(), 3);

    database = new mock_DB("a", "b", "c", "d");

    ASSERT_NO_THROW(
        {
            pool = new UpdatePool("t_update", 1);
        });

    pool->start();
    pool->push(go);
    sleep(1);

    delete pool;
    delete sock;
    freeaddrinfo(ai);
}
