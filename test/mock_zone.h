#ifndef __INC_MOCK_ZONE_H__
#define __INC_MOCK_ZONE_H__

#include "../server/classes/zone.h"

int send_nearby_objects_count = 0;
Octree *sector_contains_result = NULL;
glm::ivec3 which_sector_result(0, 0, 0);

class fake_Zone : public Zone
{
  public:
    fake_Zone(uint64_t a, uint16_t b, DB *c) : Zone(a, b, c) {};
    virtual ~fake_Zone() {};

    virtual void send_nearby_objects(uint64_t a)
        {
            ++send_nearby_objects_count;
        };

    virtual Octree *sector_contains(glm::dvec3& a)
        {
            return sector_contains_result;
        };
    virtual glm::ivec3 which_sector(glm::dvec3& a)
        {
            return which_sector_result;
        };
};

#endif /* __INC_MOCK_ZONE_H__ */
