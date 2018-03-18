#include <tap++.h>

using namespace TAP;

#include "../server/classes/motion_pool.h"

#include "mock_db.h"
#include "mock_zone.h"
#include "mock_server_globals.h"

Octree *sector;

/* Creation and deletion are uninteresting, so we needn't test them
 * individually.
 */

void test_start_stop(void)
{
    std::string test = "start/stop: ";
    motion_pool = new MotionPool("t_motion", 1);
    motion_pool->start();
    is(motion_pool->startup_arg == motion_pool, true,
       test + "expected startup arg");
    motion_pool->stop();
    delete motion_pool;
}

void test_operate(void)
{
    std::string test = "operate: ";
    std::streambuf *orig_rdbuf = std::clog.rdbuf(new Log("blah", LOG_DAEMON));
    glm::dvec3 min(0.0, 0.0, 0.0), max(1000.0, 1000.0, 1000.0);
    sector = new Octree(NULL, min, max, 1);
    database = new fake_DB("a", "b", "c", "d");

    zone = new fake_Zone(1000LL, 1, database);

    sector_contains_result = sector;

    motion_pool = new MotionPool("t_motion", 1);
    update_pool = new UpdatePool("mot_test", 1);
    GameObject *go1 = new GameObject(NULL, NULL, 9876LL);
    GameObject *go2 = new GameObject(NULL, NULL, 9877LL);
    GameObject *go3 = new GameObject(NULL, NULL, 9878LL);
    GameObject *go4 = new GameObject(NULL, NULL, 12345LL);
    GameObject *go5 = new GameObject(NULL, NULL, 12346LL);
    GameObject *go6 = new GameObject(NULL, NULL, 12347LL);

    go1->position = { 234.0, 234.0, 234.0 };
    go1->movement = { 1.0, 1.0, 1.0 };
    gettimeofday(&go1->last_updated, NULL);
    motion_pool->push(go1);
    is(motion_pool->queue_size(), 1, test + "expected queue size");
    go2->position = { 123.0, 123.0, 123.0 };
    go2->movement = { 0.0, 1.0, 1.0 };
    gettimeofday(&go2->last_updated, NULL);
    motion_pool->push(go2);
    is(motion_pool->queue_size(), 2, test + "expected queue size");
    go3->position = { 12.0, 12.0, 12.0 };
    go3->movement = { 0.0, 0.0, 1.0 };
    gettimeofday(&go3->last_updated, NULL);
    motion_pool->push(go3);
    is(motion_pool->queue_size(), 3, test + "expected queue size");
    go4->position = { 0.0, 0.0, 0.0 };
    go4->rotation = { 1.0, 0.0, 0.0 };
    gettimeofday(&go4->last_updated, NULL);
    motion_pool->push(go4);
    is(motion_pool->queue_size(), 4, test + "expected queue sizer");
    go5->position = { 1.0, 1.0, 1.0 };
    go5->rotation = { 0.0, 1.0, 0.0 };
    gettimeofday(&go5->last_updated, NULL);
    motion_pool->push(go5);
    is(motion_pool->queue_size(), 5, test + "expected queue size");
    go6->position = { 2.0, 2.0, 2.0 };
    go6->rotation = { 0.0, 0.0, 1.0 };
    gettimeofday(&go6->last_updated, NULL);
    motion_pool->push(go6);
    is(motion_pool->queue_size(), 6, test + "expected queue size");

    motion_pool->start();
    while (go3->position.z < 12.25)
        ;
    motion_pool->stop();

    is(go1->position.x > 234.0, true, test + "x pos increased");
    is(go1->position.y > 234.0, true, test + "y pos increased");
    is(go1->position.z > 234.0, true, test + "z pos increased");
    is(go2->position.y > 123.0, true, test + "y pos increased");
    is(go2->position.z > 123.0, true, test + "z pos increased");
    is(go3->position.z > 12.0, true, test + "z pos increased");
    /* Nothing happens with rotation yet, so no need to check anything
     * for go4-6.
     */

    delete update_pool;
    delete motion_pool;
    delete (fake_Zone *)zone;
    delete (fake_DB *)database;
    delete sector;
    delete std::clog.rdbuf(orig_rdbuf);
}

int main(int argc, char **argv)
{
    plan(13);

    test_start_stop();
    test_operate();
    return exit_status();
}
