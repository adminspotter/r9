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
    GameObject *go1 = new GameObject(NULL, NULL, 9876LL);
    GameObject *go2 = new GameObject(NULL, NULL, 9877LL);
    GameObject *go3 = new GameObject(NULL, NULL, 9878LL);
    GameObject *go4 = new GameObject(NULL, NULL, 12345LL);
    GameObject *go5 = new GameObject(NULL, NULL, 12346LL);
    GameObject *go6 = new GameObject(NULL, NULL, 12347LL);

    go1->position = { 234.0, 234.0, 234.0 };
    go1->movement = { 1.0, 1.0, 1.0 };
    gettimeofday(&go1->last_updated, NULL);
    pool->push(go1);
    ASSERT_EQ(pool->queue_size(), 1);
    go2->position = { 123.0, 123.0, 123.0 };
    go2->movement = { 0.0, 1.0, 1.0 };
    gettimeofday(&go2->last_updated, NULL);
    pool->push(go2);
    ASSERT_EQ(pool->queue_size(), 2);
    go3->position = { 12.0, 12.0, 12.0 };
    go3->movement = { 0.0, 0.0, 1.0 };
    gettimeofday(&go3->last_updated, NULL);
    pool->push(go3);
    ASSERT_EQ(pool->queue_size(), 3);
    go4->position = { 0.0, 0.0, 0.0 };
    go4->rotation = { 1.0, 0.0, 0.0 };
    gettimeofday(&go4->last_updated, NULL);
    pool->push(go4);
    ASSERT_EQ(pool->queue_size(), 4);
    go5->position = { 1.0, 1.0, 1.0 };
    go5->rotation = { 0.0, 1.0, 0.0 };
    gettimeofday(&go5->last_updated, NULL);
    pool->push(go5);
    ASSERT_EQ(pool->queue_size(), 5);
    go6->position = { 2.0, 2.0, 2.0 };
    go6->rotation = { 0.0, 0.0, 1.0 };
    gettimeofday(&go6->last_updated, NULL);
    pool->push(go6);
    ASSERT_EQ(pool->queue_size(), 6);

    delete zone->motion_pool;
    zone->motion_pool = pool;
    zone->motion_pool->startup_arg = (void *)zone;
    zone->motion_pool->start(MotionPool::motion_pool_worker);
    sleep(1);
    zone->motion_pool->stop();

    ASSERT_GT(go1->position.x, 234.0);
    ASSERT_GT(go1->position.y, 234.0);
    ASSERT_GT(go1->position.z, 234.0);
    ASSERT_GT(go2->position.y, 123.0);
    ASSERT_GT(go2->position.z, 123.0);
    ASSERT_GT(go3->position.z, 12.0);
    /* Nothing happens with rotation yet, so no need to check anything
     * for go4-6.
     */

    delete zone;
}
