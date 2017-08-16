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
int ntoh_count, register_count, unregister_count, action_count;

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

int ntoh_packet(packet *p, size_t s)
{
    ++ntoh_count;
    return 1;
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

TEST_F(ActionPoolTest, StartStop)
{
    EXPECT_CALL(*((mock_DB *)database), get_server_skills(_));
    EXPECT_CALL(*((mock_Library *)lib), symbol(_))
        .WillOnce(Return((void *)register_actions))
        .WillOnce(Return((void *)unregister_actions));

    action_pool = new ActionPool(1, *game_objs, lib, database);
    action_pool->start();
    ASSERT_TRUE(action_pool->startup_arg == action_pool);
    ASSERT_TRUE(action_pool->pool_size() == 1);
    action_pool->stop();
    ASSERT_TRUE(action_pool->pool_size() == 0);
    delete action_pool;
}

TEST_F(ActionPoolTest, NoSkill)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);

    EXPECT_CALL(*((mock_DB *)database), get_server_skills(_));
    EXPECT_CALL(*((mock_Library *)lib), symbol(_))
        .WillOnce(Return((void *)register_actions))
        .WillOnce(Return((void *)unregister_actions));

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
    delete action_pool;
}

TEST_F(ActionPoolTest, InvalidSkill)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[567] = {567, 5, 0, 0};

    EXPECT_CALL(*((mock_DB *)database), get_server_skills(_));
    EXPECT_CALL(*((mock_Library *)lib), symbol(_))
        .WillOnce(Return((void *)register_actions))
        .WillOnce(Return((void *)unregister_actions));

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
    delete action_pool;
}

TEST_F(ActionPoolTest, WrongObjectId)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[789] = {789, 5, 0, 0};

    EXPECT_CALL(*((mock_DB *)database), get_server_skills(_));
    EXPECT_CALL(*((mock_Library *)lib), symbol(_))
        .WillOnce(Return((void *)register_actions))
        .WillOnce(Return((void *)unregister_actions));

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
    delete action_pool;
}

TEST_F(ActionPoolTest, GoodObjectId)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[789] = {789, 5, 0, 0};

    EXPECT_CALL(*((mock_DB *)database), get_server_skills(_));
    EXPECT_CALL(*((mock_Library *)lib), symbol(_))
        .WillOnce(Return((void *)register_actions))
        .WillOnce(Return((void *)unregister_actions));

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
    delete action_pool;
}

TEST_F(ActionPoolTest, Worker)
{
    base_user *bu = new base_user(123LL, (*game_objs)[9876LL], listensock);
    bu->actions[789] = {789, 5, 0, 0};

    EXPECT_CALL(*((mock_DB *)database), get_server_skills(_));
    EXPECT_CALL(*((mock_Library *)lib), symbol(_))
        .WillOnce(Return((void *)register_actions))
        .WillOnce(Return((void *)unregister_actions));

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

    action_count = ntoh_count = 0;

    action_pool->start();

    while (action_count == 0)
        ;

    action_pool->stop();
    ASSERT_EQ(action_count, 1);
    ASSERT_GT(ntoh_count, 0);

    (*game_objs)[9876LL]->disconnect(bu);
    delete bu;
    delete action_pool;
}
