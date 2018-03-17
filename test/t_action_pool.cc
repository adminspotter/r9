#include <tap++.h>

using namespace TAP;

#include "../server/classes/action_pool.h"
#include "../server/classes/game_obj.h"

#include <gtest/gtest.h>

#include "mock_db.h"
#include "mock_library.h"
#include "mock_listensock.h"
#include "mock_server_globals.h"

void register_actions(std::map<uint16_t, action_rec>&);
void unregister_actions(std::map<uint16_t, action_rec>&);
int fake_action(GameObject *, int, GameObject *, glm::dvec3&);

fake_Library *lib;
std::map<uint64_t, GameObject *> *game_objs;
fake_listen_socket *listensock;
int register_count, unregister_count, action_count;

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

void register_actions(std::map<uint16_t, action_rec>& a)
{
    ++register_count;

    a[567].valid = false;
    a[567].def = 789;

    a[789].valid = true;
    a[789].action = &fake_action;
}

void unregister_actions(std::map<uint16_t, action_rec>& a)
{
    ++unregister_count;

    a.erase(567);
    a.erase(789);
}

int fake_action(GameObject *a, int b, GameObject *c, glm::dvec3& d)
{
    ++action_count;

    return 4;
}

void setup_fixture(void)
{
    database = new fake_DB("a", "b", "c", "d");

    /* The action pool takes control of this library, and
     * deletes it when the pool is destroyed.  We don't need
     * to delete it ourselves.
     */
    lib = new fake_Library("doesn't matter");

    symbol_count = 0;
    symbol_result = (void *)register_actions;

    game_objs = new std::map<uint64_t, GameObject *>();
    (*game_objs)[9876LL] = new GameObject(NULL, NULL, 9876LL);

    struct addrinfo *addr = create_addrinfo();
    listensock = new fake_listen_socket(addr);
    freeaddrinfo(addr);
}

void cleanup_fixture(void)
{
    close(listensock->sock.sock);
    delete listensock;
    delete (*game_objs)[9876LL];
    delete game_objs;
    delete (fake_DB *)database;
}

class ActionPoolTest : public ::testing::Test
{
  protected:
    void SetUp()
        {
            setup_fixture();
        };

    void TearDown()
        {
            cleanup_fixture();
        };
};

/* In testing the library itself, we know that it will throw an
 * exception when it can't find a symbol, so that's not interesting to
 * test in context of the ActionPool constructor.
 */
void test_create_delete(void)
{
    std::string test = "create/delete: ";

    setup_fixture();

    get_server_skills_count = register_count = unregister_count = 0;

    try
    {
        action_pool = new ActionPool(1, *game_objs, lib, database);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(get_server_skills_count, 1, test + "expected skills calls");
    is(register_count, 1, test + "expected register calls");

    symbol_result = (void *)unregister_actions;

    delete action_pool;
    is(unregister_count, 1, test + "expected unregister calls");

    cleanup_fixture();
}

TEST_F(ActionPoolTest, StartStop)
{
    action_pool = new ActionPool(1, *game_objs, lib, database);
    action_pool->start();
    ASSERT_TRUE(action_pool->startup_arg == action_pool);
    ASSERT_TRUE(action_pool->pool_size() == 1);
    action_pool->stop();
    ASSERT_TRUE(action_pool->pool_size() == 0);

    symbol_result = (void *)unregister_actions;

    delete action_pool;
}

TEST_F(ActionPoolTest, NoSkill)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.sequence = 1LL;
    pkt.object_id = 9876LL;
    pkt.action_id = 12345;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    ASSERT_EQ(action_count, 0);

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;
}

TEST_F(ActionPoolTest, InvalidSkill)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[567] = {567, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.sequence = 1LL;
    pkt.object_id = 9876LL;
    pkt.action_id = 567;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    ASSERT_EQ(action_count, 1);

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;
}

TEST_F(ActionPoolTest, WrongObjectId)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[789] = {789, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.sequence = 1LL;
    pkt.object_id = 123LL;
    pkt.action_id = 789;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    ASSERT_EQ(action_count, 0);

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;
}

TEST_F(ActionPoolTest, GoodObjectId)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[789] = {789, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    action_request pkt;
    memset(&pkt, 0, sizeof(action_request));
    pkt.type = TYPE_ACTREQ;
    pkt.version = 1;
    pkt.sequence = 1LL;
    pkt.object_id = 9876LL;
    pkt.action_id = 789;
    pkt.power_level = 5;

    action_count = 0;

    action_pool->execute_action(bu, pkt);
    ASSERT_EQ(action_count, 1);

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;
}

TEST_F(ActionPoolTest, Worker)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[789] = {789, 5, 0, 0};

    action_pool = new ActionPool(1, *game_objs, lib, database);

    packet_list pl;
    memset(&pl.buf, 0, sizeof(action_request));
    pl.buf.act.type = TYPE_ACTREQ;
    pl.buf.act.version = 1;
    pl.buf.act.sequence = 1LL;
    pl.buf.act.object_id = 9876LL;
    pl.buf.act.action_id = 789;
    pl.buf.act.power_level = 5;
    pl.who = bu;
    action_pool->push(pl);

    action_count = 0;

    action_pool->start();

    while (action_count == 0)
        ;

    action_pool->stop();
    ASSERT_EQ(action_count, 1);

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;

    symbol_result = (void *)unregister_actions;

    delete action_pool;
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(3);

    int gtests = RUN_ALL_TESTS();

    test_create_delete();
    return gtests & exit_status();
}
