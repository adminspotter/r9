#include "../server/classes/zone.h"

#include <gtest/gtest.h>

#include "mock_db.h"
#include "mock_server_globals.h"

using ::testing::_;
using ::testing::Invoke;

int fake_server_objects(std::map<uint64_t, GameObject *>& gom)
{
    glm::dvec3 pos(100.0, 100.0, 100.0);

    gom[1234LL] = new GameObject(NULL, NULL, 1234LL);
    gom[1234LL]->position = pos;

    gom[1235LL] = new GameObject(NULL, NULL, 1235LL);
    pos.x = 125.0;
    gom[1235LL]->position = pos;

    return 2;
}

class ZoneTest : public ::testing::Test
{
  protected:
    void SetUp()
        {
            database = new mock_DB("a", "b", "c", "d");
        };

    void TearDown()
        {
            delete database;
        };
};

TEST_F(ZoneTest, CreateSimple)
{
    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_));

    ASSERT_NO_THROW(
        {
            zone = new Zone(1000, 1, database);
        });
    ASSERT_TRUE(zone->game_objects.size() == 0);

    delete zone;
}

TEST_F(ZoneTest, CreateComplex)
{
    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_))
        .WillOnce(Invoke(fake_server_objects));

    ASSERT_NO_THROW(
        {
            zone = new Zone(1000, 2000, 3000, 1, 2, 3, database);
        });
    ASSERT_TRUE(zone->game_objects.size() > 0);

    delete zone;
}

TEST_F(ZoneTest, SectorMethods)
{
    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_));

    zone = new Zone(1000, 1000, 1000, 1, 1, 3, database);

    glm::dvec3 where1(500.0, 500.0, 500.0);
    glm::dvec3 where2(500.0, 500.0, 1500.0);
    Octree *o1 = zone->sector_contains(where1);
    Octree *o2 = zone->sector_contains(where2);
    ASSERT_FALSE(o1 == o2);

    glm::ivec3 which1 = zone->which_sector(where1);
    glm::ivec3 expected1(0, 0, 0);
    ASSERT_TRUE(which1 == expected1);

    glm::ivec3 which2 = zone->which_sector(where2);
    glm::ivec3 expected2(0, 0, 1);
    ASSERT_TRUE(which2 == expected2);

    delete zone;
}

TEST_F(ZoneTest, SendObjects)
{
    int obj_size, queue_size;

    EXPECT_CALL(*((mock_DB *)database), get_server_objects(_))
        .WillOnce(Invoke(fake_server_objects));

    zone = new Zone(1000, 1, database);
    obj_size = zone->game_objects.size();
    ASSERT_GT(obj_size, 0);

    update_pool = new UpdatePool("zonetest", 1);

    zone->send_nearby_objects(1234LL);
    ASSERT_TRUE(zone->game_objects.size() == obj_size);

    /* Update queue will have length of this object, plus all other
     * objects "within visual range".  In our case here, it will be 2.
     */
    queue_size = update_pool->queue_size();
    ASSERT_GT(queue_size, 1);

    zone->send_nearby_objects(9876LL);
    ASSERT_TRUE(zone->game_objects.size() > obj_size);
    ASSERT_TRUE(update_pool->queue_size() > queue_size);

    delete update_pool;
    delete zone;
}

