#include "../server/classes/modules/db.h"

#include <gtest/gtest.h>

class fake_DB : public DB
{
    fake_DB(const std::string& a, const std::string& b,
            const std::string& c, const std::string& d) : DB(a, b, c, d) {};
    virtual ~fake_DB() {};

    /* Player functions */
    virtual uint64_t check_authentication(const std::string& a,
                                          const std::string& b)
        {
            return 0LL;
        };
    virtual int check_authorization(uint64_t a, uint64_t b)
        {
            return 0;
        };
    virtual uint64_t get_character_objectid(uint64_t a, const std::string& b)
        {
            return 0LL;
        };
    virtual int open_new_login(uint64_t a, uint64_t b, Sockaddr *c)
        {
            return 0;
        };
    virtual int check_open_login(uint64_t a, uint64_t b)
        {
            return 0;
        };
    virtual int close_open_login(uint64_t a, uint64_t b, Sockaddr *c)
        {
            return 0;
        };
    virtual int get_player_server_skills(uint64_t a, uint64_t b,
                                         std::map<uint16_t,
                                         action_level>& c)
        {
            return 0;
        };

    /* Server functions */
    virtual int get_server_skills(std::map<uint16_t, action_rec>& a)
        {
            return 0;
        };
    virtual int get_server_objects(std::map<uint64_t, GameObject *>& a)
        {
            return 0;
        };
};
