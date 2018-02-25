#ifndef __INC_MOCK_ZONE_H__
#define __INC_MOCK_ZONE_H__

#include "../server/classes/zone.h"

class mock_Zone : public Zone
{
  public:
    mock_Zone(uint64_t a, uint16_t b, DB *c) : Zone(a, b, c) {};
    virtual ~mock_Zone() {};

    MOCK_METHOD1(send_nearby_objects, void(uint64_t));

    MOCK_METHOD1(sector_contains, Octree *(glm::dvec3&));
    MOCK_METHOD1(which_sector, glm::ivec3(glm::dvec3&));
};

#endif /* __INC_MOCK_ZONE_H__ */
