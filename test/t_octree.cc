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

TEST(OctreeTest, BuildEmptyList)
{
    std::list<GameObject *> objs;
    glm::dvec3 min = {0.0, 0.0, 0.0}, max = {100.0, 100.0, 100.0};
    Octree *tree = new Octree(NULL, min, max, 0);

    ASSERT_NO_THROW(
        {
            tree->build(objs);
        });

    ASSERT_TRUE(tree->empty() == true);

    Octree *sub = tree->octants[0];
    ASSERT_TRUE(sub != NULL);
    while (sub->octants[0] != NULL)
        sub = sub->octants[0];
    ASSERT_EQ(sub->parent_index, 0);
    ASSERT_EQ(sub->depth, Octree::MIN_DEPTH);

    delete tree;
}
