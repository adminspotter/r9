#ifndef __INC_MOCK_DB_H__
#define __INC_MOCK_DB_H__

#include "../server/classes/modules/db.h"

const char address[] = "1.2.3.4";

/* Some hosts perform these next two operations veeeeeery sloooooowly,
 * so we'll include some VERY FAST versions.  The database object uses
 * these in its startup method.
 */
int gethostname(char *a, size_t b)
{
    strncpy(a, "whut.foo.com", b);
    return 0;
}

const char *inet_ntop(int a, const void *b, char *c, socklen_t d)
{
    return address;
}

int check_authentication_count = 0;
uint64_t check_authentication_result = 12345LL;
int check_authorization_count = 0, check_authorization_result = 0;
int get_characterid_count = 0;
uint64_t get_characterid_result = 0LL;
int get_character_objectid_count = 0;
uint64_t get_character_objectid_result = 0LL;
int get_player_server_skills_count = 0, get_player_server_skills_result = 0;
int get_server_skills_count = 0, get_server_skills_result = 0;
int get_server_objects_count = 0, get_server_objects_result = 0;

class fake_DB : public DB
{
  public:
    fake_DB(const std::string& a, const std::string& b,
            const std::string& c, const std::string& d) : DB(a, b, c, d) {};
    virtual ~fake_DB() {};

    virtual uint64_t check_authentication(const std::string& a,
                                          const uint8_t *b,
                                          size_t c)
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
    virtual int get_player_server_skills(uint64_t a, uint64_t b,
                                         std::map<uint16_t, action_level>& c)
        {
            ++get_player_server_skills_count;
            return get_player_server_skills_result;
        };
    virtual int get_server_skills(std::map<uint16_t, action_rec>& a)
        {
            ++get_server_skills_count;
            return get_server_skills_result;
        };
    virtual int get_server_objects(std::map<uint64_t, GameObject *>& a)
        {
            ++get_server_objects_count;
            return get_server_objects_result;
        };
};

#endif /* __INC_MOCK_DB_H__ */
