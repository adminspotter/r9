#include "../server/classes/modules/db.h"

#include <gtest/gtest.h>

bool gethostname_failure = false, getaddrinfo_failure = false;
bool ntop_failure = false;
const char address[] = "1.2.3.4";

int gethostname(char *a, size_t b)
{
    if (gethostname_failure == true)
    {
        errno = EPERM;
        return -1;
    }
    strncpy(a, "whut.foo.com", b);
    return 0;
}

/* This function will leak memory like crazy, but it's just a test and
 * will terminate quickly and never do anything else, so it's just not
 * important.
 */
int getaddrinfo(const char *a, const char *b,
                const struct addrinfo *c, struct addrinfo **d)
{
    if (getaddrinfo_failure == true)
        return EAI_BADFLAGS;
    *d = new struct addrinfo;
    (*d)->ai_family = AF_INET;
    (*d)->ai_addr = (struct sockaddr *)new struct sockaddr_in;
    ((struct sockaddr_in *)(*d)->ai_addr)->sin_addr.s_addr = 1;
    return 0;
}

void freeaddrinfo(struct addrinfo *a)
{
    delete a->ai_addr;
    delete a;
}

const char *inet_ntop(int a, const void *b, char *c, socklen_t d)
{
    if (ntop_failure == true)
        return NULL;
    return address;
}

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
