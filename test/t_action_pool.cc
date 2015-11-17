#include "../server/classes/action_pool.h"
#include "../server/classes/zone.h"
#include "../server/classes/modules/db.h"
#include "../proto/proto.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;

std::vector<listen_socket *> sockets;

class mock_Zone : public Zone
{
  public:
    mock_Zone(uint64_t a, uint16_t b) : Zone(a, b) {};
    mock_Zone(uint64_t a, uint64_t b, uint64_t c,
              uint16_t d, uint16_t e, uint16_t f) : Zone(a, b, c, d, e, f) {};
    ~mock_Zone() {};

    MOCK_METHOD3(execute_action, void(Control *a, action_request& b, size_t c));
};

DB *database;

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
    pkt.who = (uint64_t)control;

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
    pkt.who = (uint64_t)control;

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
    pkt.who = (uint64_t)control;

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
    pkt.who = (uint64_t)control;

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
