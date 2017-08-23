#include "../server/classes/octree.h"

#include <gtest/gtest.h>

TEST(OctreeTest, CreateDelete)
{
    Octree *tree = NULL;
    glm::dvec3 min = {0.0, 0.0, 0.0}, max = {100.0, 100.0, 100.0};

    ASSERT_NO_THROW(
        {
            tree = new Octree(NULL, min, max, 0);
        });

    ASSERT_EQ(tree->min_point, min);
    ASSERT_EQ(tree->max_point, max);

    ASSERT_NO_THROW(
        {
            delete tree;
        });
}
