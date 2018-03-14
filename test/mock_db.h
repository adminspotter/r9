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
    MOCK_METHOD2(check_authorization, int(uint64_t, const std::string&));
    MOCK_METHOD2(get_characterid, uint64_t(uint64_t, const std::string&));
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

int check_authentication_count = 0;
uint64_t check_authentication_result = 12345LL;
int check_authorization_count = 0, check_authorization_result = 0;
int get_characterid_count = 0;
uint64_t get_characterid_result = 0LL;
int get_character_objectid_count = 0;
uint64_t get_character_objectid_result = 0LL;
int open_new_login_result = 0, check_open_login_result = 0;
int close_open_login_result = 0;
int get_player_server_skills_count = 0, get_player_server_skills_result = 0;
int get_server_skills_result = 0, get_server_objects_result = 0;

class fake_DB : public DB
{
  public:
    fake_DB(const std::string& a, const std::string& b,
            const std::string& c, const std::string& d) : DB(a, b, c, d) {};
    virtual ~fake_DB() {};

    virtual uint64_t check_authentication(const std::string& a,
                                          const std::string& b)
        {
            ++check_authentication_count;
            return check_authentication_result;
        };
    virtual int check_authorization(uint64_t a, uint64_t b)
        {
            return check_authorization_result;
        };
    virtual int check_authorization(uint64_t a, const std::string& b)
        {
            ++check_authorization_count;
            return check_authorization_result;
        };
    virtual uint64_t get_characterid(uint64_t a, const std::string& b)
        {
            ++get_characterid_count;
            return get_characterid_result;
        };
    virtual uint64_t get_character_objectid(uint64_t a,
                                            const std::string& b)
        {
            ++get_character_objectid_count;
            return get_character_objectid_result;
        };
    virtual int open_new_login(uint64_t a, uint64_t b, Sockaddr *c)
        {
            return open_new_login_result;
        };
    virtual int check_open_login(uint64_t a, uint64_t b)
        {
            return check_open_login_result;
        };
    virtual int close_open_login(uint64_t a, uint64_t b, Sockaddr *c)
        {
            return close_open_login_result;
        };
    virtual int get_player_server_skills(uint64_t a, uint64_t b,
                                         std::map<uint16_t, action_level>& c)
        {
            ++get_player_server_skills_count;
            return get_player_server_skills_result;
        };
    virtual int get_server_skills(std::map<uint16_t, action_rec>& a)
        {
            return get_server_skills_result;
        };
    virtual int get_server_objects(std::map<uint64_t, GameObject *>& a)
        {
            return get_server_objects_result;
        };
};

#endif /* __INC_MOCK_DB_H__ */
