#include "../server/classes/update_pool.h"
#include "../server/classes/listensock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_listensock.h"
#include "mock_server_globals.h"

using ::testing::_;

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

TEST(UpdatePoolTest, Operate)
{
    struct addrinfo hints, *ai;
    int ret;
    listen_socket *sock;
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
            sock = new mock_listen_socket(ai);
        });
    delete sock->send_pool;
    sock->send_pool = send_pool;
    sockets.push_back(sock);

    sock->users[1LL] = new base_user(1LL, NULL, NULL);
    sock->users[2LL] = new base_user(2LL, NULL, NULL);
    sock->users[3LL] = new base_user(3LL, NULL, NULL);
    ASSERT_EQ(sock->users.size(), 3);

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
