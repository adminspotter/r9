#include "../server/classes/zone.h"

#include <gtest/gtest.h>

#include "mock_db.h"
#include "mock_server_globals.h"

using ::testing::_;
using ::testing::Invoke;

int fake_server_objects(std::map<uint64_t, GameObject *>& gom)
{
    gom[1234LL] = new GameObject(NULL, NULL, 1234LL);
    gom[1235LL] = new GameObject(NULL, NULL, 1235LL);
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
