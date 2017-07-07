#ifndef __INC_MOCK_DB_H__
#define __INC_MOCK_DB_H__

#include <gmock/gmock.h>

#include "../server/classes/modules/db.h"

class mock_DB : public DB
{
  public:
    mock_DB(const std::string& a, const std::string& b,
            const std::string& c, const std::string& d) : DB(a, b, c, d) {};
    virtual ~mock_DB() {};

    MOCK_METHOD2(check_authentication, uint64_t(const std::string&,
                                                const std::string&));
    MOCK_METHOD2(check_authorization, int(uint64_t, uint64_t));
    MOCK_METHOD2(get_character_objectid, uint64_t(uint64_t,
                                                  const std::string&));
    MOCK_METHOD3(open_new_login, int(uint64_t, uint64_t, Sockaddr *));
    MOCK_METHOD2(check_open_login, int(uint64_t, uint64_t));
    MOCK_METHOD3(close_open_login, int(uint64_t, uint64_t, Sockaddr *));
    MOCK_METHOD3(get_player_server_skills, int(uint64_t, uint64_t,
                                               std::map<uint16_t,
                                               action_level>&));
    MOCK_METHOD1(get_server_skills, int(std::map<uint16_t, action_rec>&));
    MOCK_METHOD1(get_server_objects, int(std::map<uint64_t, GameObject *>&));
};

#endif /* __INC_MOCK_DB_H__ */
