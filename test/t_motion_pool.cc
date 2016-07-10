#include "../server/classes/motion_pool.h"
#include "../server/classes/zone.h"
#include "../server/classes/game_obj.h"
#include "../server/classes/modules/db.h"

#include <gtest/gtest.h>

std::vector<listen_socket *> sockets;
DB *database;

TEST(MotionPoolTest, Operate)
{
    Zone *zone = new Zone(1000LL, 1);
    MotionPool *pool = new MotionPool("t_motion", 1);
    GameObject *go = new GameObject(NULL, NULL, 9876LL);

    go->position = { 234.0, 234.0, 234.0 };
    go->movement = { 1.0, 1.0, 1.0 };
    gettimeofday(&go->last_updated, NULL);
    pool->push(go);
    ASSERT_EQ(pool->queue_size(), 1);

    delete zone->motion_pool;
    zone->motion_pool = pool;
    zone->motion_pool->startup_arg = (void *)zone;
    zone->motion_pool->start(MotionPool::motion_pool_worker);
    sleep(1);
    zone->motion_pool->stop();

    ASSERT_GT(go->position.x, 234.0);
    ASSERT_GT(go->position.y, 234.0);
    ASSERT_GT(go->position.z, 234.0);

    delete zone;
}
