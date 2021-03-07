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

    go1->set_position(glm::dvec3(234.0, 234.0, 234.0));
    go1->set_movement(glm::dvec3(1.0, 1.0, 1.0));
    motion_pool->push(go1);
    is(motion_pool->queue_size(), 1, test + "expected queue size");
    go2->set_position(glm::dvec3(123.0, 123.0, 123.0));
    go2->set_movement(glm::dvec3(0.0, 1.0, 1.0));
    motion_pool->push(go2);
    is(motion_pool->queue_size(), 2, test + "expected queue size");
    go3->set_position(glm::dvec3(12.0, 12.0, 12.0));
    go3->set_movement(glm::dvec3(0.0, 0.0, 1.0));
    motion_pool->push(go3);
    is(motion_pool->queue_size(), 3, test + "expected queue size");
    go4->set_position(glm::dvec3(0.0, 0.0, 0.0));
    go4->set_rotation(glm::angleAxis(1.0, glm::dvec3(1.0, 0.0, 0.0)));
    motion_pool->push(go4);
    is(motion_pool->queue_size(), 4, test + "expected queue size");
    go5->set_position(glm::dvec3(1.0, 1.0, 1.0));
    go5->set_rotation(glm::angleAxis(1.0, glm::dvec3(0.0, 1.0, 0.0)));
    motion_pool->push(go5);
    is(motion_pool->queue_size(), 5, test + "expected queue size");
    go6->set_position(glm::dvec3(2.0, 2.0, 2.0));
    go6->set_movement(glm::dvec3(10.0, 0.0, 0.0));
    go6->deactivate();
    motion_pool->push(go6);
    is(motion_pool->queue_size(), 6, test + "expected queue size");

    motion_pool->start();
    while (go3->get_position().z < 12.25)
        ;
    motion_pool->stop();

    ok(go1->get_position().x > 234.0, test + "x pos increased");
    ok(go1->get_position().y > 234.0, test + "y pos increased");
    ok(go1->get_position().z > 234.0, test + "z pos increased");
    ok(go2->get_position().y > 123.0, test + "y pos increased");
    ok(go2->get_position().z > 123.0, test + "z pos increased");
    ok(go3->get_position().z > 12.0, test + "z pos increased");
    ok(glm::eulerAngles(go4->get_orientation()).x != 0.0,
       test + "x rot changed");
    ok(glm::eulerAngles(go5->get_orientation()).y != 0.0,
       test + "y rot changed");
    ok(go6->get_position().x == 2.0, test + "x pos stayed the same");

    delete update_pool;
    delete motion_pool;
    delete (fake_Zone *)zone;
    delete (fake_DB *)database;
    delete sector;
    delete std::clog.rdbuf(orig_rdbuf);
}

int main(int argc, char **argv)
{
    plan(16);

    test_start_stop();
    test_operate();
    return exit_status();
}
