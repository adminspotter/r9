#include "../server/classes/action_pool.h"
#include "../server/classes/game_obj.h"
#include "../proto/proto.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_db.h"
#include "mock_library.h"
#include "mock_listensock.h"
#include "mock_server_globals.h"

using ::testing::_;
using ::testing::Return;

void register_actions(std::map<uint16_t, action_rec>&);
void unregister_actions(std::map<uint16_t, action_rec>&);
int fake_action(GameObject *, int, GameObject *, glm::dvec3&);

Library *lib;
std::map<uint64_t, GameObject *> *game_objs;
mock_listen_socket *listensock;
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

class ActionPoolTest : public ::testing::Test
{
  protected:
    void SetUp()
        {
            database = new mock_DB("a", "b", "c", "d");

            lib = new mock_Library("doesn't matter");

            game_objs = new std::map<uint64_t, GameObject *>();
            (*game_objs)[9876LL] = new GameObject(NULL, NULL, 9876LL);

            struct addrinfo *addr = create_addrinfo();
            listensock = new mock_listen_socket(addr);
            freeaddrinfo(addr);
        };

    void TearDown()
        {
            close(listensock->sock.sock);
            delete listensock;
            delete (*game_objs)[9876LL];
            delete game_objs;
            delete database;
        };
};

/* In testing the library itself, we know that it will throw an
 * exception when it can't find a symbol, so that's not interesting to
 * test in context of the ActionPool constructor.
 */
TEST_F(ActionPoolTest, CreateDelete)
{
    EXPECT_CALL(*((mock_DB *)database), get_server_skills(_));
    EXPECT_CALL(*((mock_Library *)lib), symbol(_))
        .WillOnce(Return((void *)register_actions))
        .WillOnce(Return((void *)unregister_actions));

    register_count = unregister_count = 0;

    ASSERT_NO_THROW(
        {
            action_pool = new ActionPool(1, *game_objs, lib, database);
        });
    ASSERT_EQ(register_count, 1);

    delete action_pool;
    ASSERT_EQ(unregister_count, 1);
}

TEST(ActionPoolTest, NoSkill)
{
    mock_Zone *zone = new mock_Zone(1, 1);
    ActionPool *pool = new ActionPool("t_action", 1);
    GameObject *go = new GameObject(NULL, NULL, 9876LL);
    Control *control = new Control(123LL, go);
    packet_list pkt;

    EXPECT_CALL(*zone, execute_action(_, _, _)).Times(0);

    memset(&pkt.buf, 0, sizeof(action_request));
    pkt.buf.act.type = TYPE_ACTREQ;
    pkt.buf.act.version = 1;
    pkt.buf.act.sequence = 1LL;
    pkt.buf.act.object_id = 123LL;
    pkt.buf.act.action_id = 12345;
    pkt.buf.act.power_level = 5;
    hton_packet(&pkt.buf, sizeof(action_request));
    pkt.who = control;

    go->connect(control);

    delete zone->action_pool;
    zone->action_pool = pool;
    zone->action_pool->push(pkt);
    zone->action_pool->startup_arg = (void *)zone;
    zone->action_pool->start(ActionPool::action_pool_worker);
    sleep(1);

    go->disconnect(control);
    delete control;
    delete zone;
}

TEST(ActionPoolTest, InvalidSkill)
{
    mock_Zone *zone = new mock_Zone(1, 1);
    ActionPool *pool = new ActionPool("t_action", 1);
    action_rec rec;
    GameObject *go = new GameObject(NULL, NULL, 9876LL);
    Control *control = new Control(123LL, go);
    packet_list pkt;

    EXPECT_CALL(*zone, execute_action(_, _, _)).Times(0);

    rec.valid = false;
    zone->actions[12345] = rec;

    memset(&pkt.buf, 0, sizeof(action_request));
    pkt.buf.act.type = TYPE_ACTREQ;
    pkt.buf.act.version = 1;
    pkt.buf.act.sequence = 1LL;
    pkt.buf.act.object_id = 123LL;
    pkt.buf.act.action_id = 12345;
    pkt.buf.act.power_level = 5;
    hton_packet(&pkt.buf, sizeof(action_request));
    pkt.who = control;

    go->connect(control);

    delete zone->action_pool;
    zone->action_pool = pool;
    zone->action_pool->push(pkt);
    zone->action_pool->startup_arg = (void *)zone;
    zone->action_pool->start(ActionPool::action_pool_worker);
    sleep(1);

    go->disconnect(control);
    delete control;
    delete zone;
}

TEST(ActionPoolTest, WrongObjectId)
{
    mock_Zone *zone = new mock_Zone(1, 1);
    ActionPool *pool = new ActionPool("t_action", 1);
    action_rec rec;
    GameObject *go = new GameObject(NULL, NULL, 9876LL);
    Control *control = new Control(123LL, go);
    packet_list pkt;

    EXPECT_CALL(*zone, execute_action(_, _, _)).Times(0);

    rec.valid = true;
    zone->actions[12345] = rec;

    memset(&pkt.buf, 0, sizeof(action_request));
    pkt.buf.act.type = TYPE_ACTREQ;
    pkt.buf.act.version = 1;
    pkt.buf.act.sequence = 1LL;
    pkt.buf.act.object_id = 123LL;
    pkt.buf.act.action_id = 12345;
    pkt.buf.act.power_level = 5;
    hton_packet(&pkt.buf, sizeof(action_request));
    pkt.who = control;

    go->connect(control);

    delete zone->action_pool;
    zone->action_pool = pool;
    zone->action_pool->push(pkt);
    zone->action_pool->startup_arg = (void *)zone;
    zone->action_pool->start(ActionPool::action_pool_worker);
    sleep(1);

    go->disconnect(control);
    delete control;
    delete zone;
}

TEST(ActionPoolTest, GoodObjectId)
{
    mock_Zone *zone = new mock_Zone(1, 1);
    ActionPool *pool = new ActionPool("t_action", 1);
    action_rec rec;
    GameObject *go = new GameObject(NULL, NULL, 9876LL);
    Control *control = new Control(123LL, go);
    packet_list pkt;

    EXPECT_CALL(*zone, execute_action(_, _, _));

    rec.valid = true;
    zone->actions[12345] = rec;

    memset(&pkt.buf, 0, sizeof(action_request));
    pkt.buf.act.type = TYPE_ACTREQ;
    pkt.buf.act.version = 1;
    pkt.buf.act.sequence = 1LL;
    pkt.buf.act.object_id = 9876LL;
    pkt.buf.act.action_id = 12345;
    pkt.buf.act.power_level = 5;
    hton_packet(&pkt.buf, sizeof(action_request));
    pkt.who = control;

    go->connect(control);

    delete zone->action_pool;
    zone->action_pool = pool;
    zone->action_pool->push(pkt);
    zone->action_pool->startup_arg = (void *)zone;
    zone->action_pool->start(ActionPool::action_pool_worker);
    sleep(1);

    go->disconnect(control);
    delete control;
    delete zone;
}
