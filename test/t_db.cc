#include <tap++.h>

using namespace TAP;

#include <stdexcept>

#include "../server/classes/modules/db.h"

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
  public:
    fake_DB(const std::string& a, const std::string& b,
            const std::string& c, const std::string& d) : DB(a, b, c, d) {};
    virtual ~fake_DB() {};

    /* Player functions */
    virtual uint64_t check_authentication(const std::string& a,
                                          const uint8_t *b,
                                          size_t c)
        {
            return 0LL;
        };
    virtual int check_authorization(uint64_t a, uint64_t b)
        {
            return 0;
        };
    virtual int check_authorization(uint64_t a, const std::string& b)
        {
            return 0;
        };
    virtual uint64_t get_characterid(uint64_t a, const std::string& b)
        {
            return 0LL;
        };
    virtual uint64_t get_character_objectid(uint64_t a, const std::string& b)
        {
            return 0LL;
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

void test_bad_gethostbyname(void)
{
    std::string test = "bad gethostbyname: ";
    fake_DB *database;

    gethostname_failure = true;
    try
    {
        database = new fake_DB("a", "b", "c", "d");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't get hostname"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    gethostname_failure = false;
}

void test_bad_getaddrinfo(void)
{
    std::string test = "bad getaddrinfo: ";
    fake_DB *database;

    getaddrinfo_failure = true;
    try
    {
        database = new fake_DB("a", "b", "c", "d");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't get address"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    getaddrinfo_failure = false;
}

void test_bad_ntop(void)
{
    std::string test = "bad ntop: ";
    fake_DB *database;

    ntop_failure = true;
    try
    {
        database = new fake_DB("a", "b", "c", "d");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("couldn't convert IP"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
    ntop_failure = false;
}

void test_success(void)
{
    std::string test = "success: ";
    fake_DB *database;

    try
    {
        database = new fake_DB("a", "b", "c", "d");
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    delete database;
}

int main(int argc, char **argv)
{
    plan(3);

    test_bad_gethostbyname();
    test_bad_getaddrinfo();
    test_bad_ntop();
    test_success();
    return exit_status();
}
