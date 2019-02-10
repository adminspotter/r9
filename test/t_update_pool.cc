#include <tap++.h>

using namespace TAP;

#include "../server/classes/update_pool.h"
#include "../server/classes/listensock.h"

#include "mock_listensock.h"
#include "mock_server_globals.h"

int push_count = 0;

class fake_SendPool : public ThreadPool<packet_list>
{
  public:
    fake_SendPool(const char *a, int b) : ThreadPool<packet_list>(a, b) {};
    virtual ~fake_SendPool() {};

    virtual void start(void *(*func)(void *)) {};
    virtual void push(packet_list& p)
        {
            ++push_count;
        };
};

void test_operate(void)
{
    std::string test = "operate: ";
    listen_socket *sock;
    UpdatePool *pool;
    fake_SendPool *send_pool = new fake_SendPool("t_send", 1);
    GameObject *go = new GameObject(NULL, NULL, 12345LL);

    try
    {
        sock = new fake_listen_socket(NULL);
    }
    catch (...)
    {
        fail(test + "fake listen_socket exception");
    }
    delete sock->send_pool;
    sock->send_pool = send_pool;
    sockets.push_back(sock);

    sock->users[1LL] = new base_user(1LL, NULL, NULL);
    sock->users[2LL] = new base_user(2LL, NULL, NULL);
    sock->users[3LL] = new base_user(3LL, NULL, NULL);
    is(sock->users.size(), 3, test + "expected user list size");

    try
    {
        pool = new UpdatePool("t_update", 1);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    pool->start();
    pool->push(go);
    while (push_count < 3)
        ;

    is(push_count, 3, test + "expected send push count");

    delete pool;
    delete sock;
}

int main(int argc, char **argv)
{
    plan(2);

    test_operate();
    return exit_status();
}
