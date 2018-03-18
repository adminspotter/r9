#include <tap++.h>

using namespace TAP;

#include "../server/classes/sockaddr.h"

#include <config.h>

#include <string.h>

#include <iostream>
#include <typeinfo>

/* Test addresses */
#define v4_ADDRESS  "192.168.252.16"
#define v6_ADDRESS  "fd4b:5fd9:9623:5617:dead:beef:abcd:1234"

#define HOST_NAME   "some.host.name.com"

/* Mock out getnameinfo for the hostname tests */
/* ARGSUSED */
extern "C" int getnameinfo(const struct sockaddr * __restrict sa,
                           socklen_t salen,
                           char * __restrict host, socklen_t hostlen,
                           char * __restrict serv, socklen_t servlen,
                           GETNAMEINFO_FLAGS_TYPE flags)
{
    strcpy(host, HOST_NAME);
    return 0;
}

class fake_Sockaddr : public Sockaddr
{
  public:
    fake_Sockaddr() : Sockaddr() {};
    fake_Sockaddr(const Sockaddr& s) : Sockaddr(s) {};
    fake_Sockaddr(const struct sockaddr& s) : Sockaddr(s) {};
    virtual ~fake_Sockaddr() {};

    virtual bool operator<(const Sockaddr& s) const {return false;};
    virtual bool operator<(const struct sockaddr& s) const {return false;};

    virtual const char *ntop(void) {return "howdy";};
    virtual const char *hostname(void) {return "host";};
    virtual uint16_t port(void) {return 42;};
};

void test_sockaddr_blank_constructor(void)
{
    std::string test = "sockaddr blank constructor: ";
    fake_Sockaddr *fs = new fake_Sockaddr;

    is(fs->ss.ss_family, AF_INET, test + "expected family");

    delete fs;
}

void test_sockaddr_copy_constructor(void)
{
    std::string test = "sockaddr copy constructor: ";
    fake_Sockaddr *fs = new fake_Sockaddr;

    fs->ss.ss_family = 42;

    fake_Sockaddr *fs2 = new fake_Sockaddr(*(Sockaddr *)fs);

    is(fs2->ss.ss_family, 42, test + "expected family");

    delete fs2;
    delete fs;
}

void test_sockaddr_struct_constructor(void)
{
    std::string test = "sockaddr struct constructor: ";
    struct sockaddr_in sa;

    sa.sin_family = 19;

    fake_Sockaddr *fs = new fake_Sockaddr((const struct sockaddr&)sa);

    is(fs->ss.ss_family, 19, test + "expected family");

    delete fs;
}

void test_sockaddr_equal_comparison(void)
{
    std::string test = "sockaddr equal: ";
    fake_Sockaddr *fs = new fake_Sockaddr;
    memset(&(fs->ss), 4, sizeof(struct sockaddr_storage));

    fake_Sockaddr *fs2 = new fake_Sockaddr;
    memset(&(fs2->ss), 5, sizeof(struct sockaddr_storage));

    is(*fs == *fs2, false, test + "objects not equal");

    memset(&(fs2->ss), 4, sizeof(struct sockaddr_storage));

    is(*fs == *fs2, true, test + "objects equal");
    is(*fs == ((struct sockaddr *)&fs2->ss), true,
       test + "object equal to struct");

    memset(&(fs2->ss), 0, sizeof(struct sockaddr_storage));
    memset(&(fs->ss), 0, sizeof(struct sockaddr_storage));
    delete fs2;
    delete fs;
}

void test_sockaddr_assignment(void)
{
    std::string test = "sockaddr assignment: ";
    fake_Sockaddr *fs = new fake_Sockaddr;
    memset(&(fs->ss), 4, sizeof(struct sockaddr_storage));

    fake_Sockaddr *fs2 = new fake_Sockaddr;
    memset(&(fs2->ss), 5, sizeof(struct sockaddr_storage));

    is(*fs == *fs2, false, test + "objects not equal");

    *fs = *fs2;

    is(*fs == *fs2, true, test + "objects equal");

    memset(&(fs2->ss), 0, sizeof(struct sockaddr_storage));
    memset(&(fs->ss), 0, sizeof(struct sockaddr_storage));
    delete fs2;
    delete fs;
}

void test_sockaddr_sockaddr(void)
{
    std::string test = "sockaddr: ";
    struct sockaddr_storage ss;
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    ss.ss_family = 33;

    fake_Sockaddr *sa = new fake_Sockaddr((const struct sockaddr&)ss);

    struct sockaddr_storage *saddr = (struct sockaddr_storage *)sa->sockaddr();
    is(saddr->ss_family, 33, test + "expected family");

    delete sa;
}

void test_sockaddr_factory(void)
{
    std::string test = "sockaddr factory: ";
    struct sockaddr sa;

    sa.sa_family = 0;
    while (sa.sa_family == AF_INET || sa.sa_family == AF_INET6
           || sa.sa_family == AF_UNIX)
        ++sa.sa_family;

    try
    {
        Sockaddr *s = build_sockaddr((struct sockaddr&)sa);
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        isnt(err.find("invalid address family"), std::string::npos,
             test + "correct error contents");
    }
    catch (...)
    {
        fail(test + "wrong error type");
    }
}

void test_sockaddr_in_blank_constructor(void)
{
    std::string test = "sockaddr_in blank constructor: ";
    Sockaddr_in *sa = new Sockaddr_in;

    is(sa->sin->sin_family, AF_INET, test + "expected family");
    is(sa->sin->sin_port, htons(0), test + "expected port");
    is(sa->sin->sin_addr.s_addr, htonl(INADDR_ANY), test + "expected addr");

    delete sa;
}

void test_sockaddr_in_copy_constructor(void)
{
    std::string test = "sockaddr_in copy constructor: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    Sockaddr_in *sa = new Sockaddr_in;

    sa->sin->sin_port = htons(1234);
    sa->sin->sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa2 = new Sockaddr_in(*((Sockaddr *)sa));

    is(sa2->sin->sin_family, AF_INET, test + "expected family");
    is(sa2->sin->sin_port, htons(1234), test + "expected port");
    is(sa2->sin->sin_addr.s_addr, ip_addr, test + "expected addr");

    delete sa2;
    delete sa;
}

void test_sockaddr_in_struct_constructor(void)
{
    std::string test = "sockaddr_in struct constructor: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    is(sa->sin->sin_family, AF_INET, test + "expected family");
    is(sa->sin->sin_port, htons(1234), test + "expected port");
    is(sa->sin->sin_addr.s_addr, ip_addr, test + "expected addr");

    delete sa;
}

void test_sockaddr_in_equal_comparison(void)
{
    std::string test = "sockaddr_in equal: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa1 = new Sockaddr_in((const struct sockaddr&)sin);
    Sockaddr_in *sa2 = new Sockaddr_in((const struct sockaddr&)sin);

    is(*sa1 == *sa2, true, test + "objects equal");

    sa2->sin->sin_port = htons(2345);

    is(*sa1 == *sa2, false, test + "objects not equal");

    is(*sa1 == (const struct sockaddr *)&sin, true,
       test + "object equal to struct");
    is(*sa2 == (const struct sockaddr *)&sin, false,
       test + "object not equal to struct");

    Sockaddr_in6 sa6;

    is(*sa1 == *((Sockaddr *)&sa6), false,
        test + "object not equal to different object");

    delete sa2;
    delete sa1;
}

void test_sockaddr_in_less_comparison(void)
{
    std::string test = "sockaddr_in less: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa1 = new Sockaddr_in((const struct sockaddr&)sin);
    Sockaddr_in *sa2 = new Sockaddr_in((const struct sockaddr&)sin);

    sa2->sin->sin_addr.s_addr += htonl(1L);

    is(*sa1 < *sa2, true, test + "object less than object");

    sa1->sin->sin_addr.s_addr += htonl(10L);

    is(*sa1 < *sa2, false, test + "object not less than object");
    is(*sa1 < *((const struct sockaddr *)&sin), false,
       test + "object not less than struct");

    Sockaddr_in6 sa6;

    is(*sa1 < *((Sockaddr *)&sa6), false,
       test + "object not less than different object");

    delete sa2;
    delete sa1;
}

void test_sockaddr_in_ntop(void)
{
    std::string test = "sockaddr_in ntop: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    is(strncmp(sa->ntop(), v4_ADDRESS, strlen(v4_ADDRESS)), 0,
       test + "ntop equal to addr");

    delete sa;
}

void test_sockaddr_in_hostname(void)
{
    std::string test = "sockaddr_in hostname: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    is(strcmp(sa->hostname(), HOST_NAME), 0, test + "hostnames equal");

    delete sa;
}

void test_sockaddr_in_port(void)
{
    std::string test = "sockaddr_in port: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    is(sa->port(), 1234, test + "ports equal");

    delete sa;
}

void test_sockaddr_in_sockaddr(void)
{
    std::string test = "sockaddr_in: ";
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    struct sockaddr_in *saddr = (struct sockaddr_in *)sa->sockaddr();
    is(saddr->sin_family, AF_INET, test + "expected family");
    is(saddr->sin_port, htons(1234), test + "expected port");
    is(saddr->sin_addr.s_addr, ip_addr, test + "expected addr");

    delete sa;
}

void test_sockaddr_in_factory(void)
{
    std::string test = "sockaddr_in factory: ";
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    int ret = inet_pton(AF_INET, v4_ADDRESS, &sin.sin_addr);
    is(ret, 1, test + "pton successful");

    Sockaddr *sa = build_sockaddr((struct sockaddr&)sin);
    Sockaddr_in *sa_in = new Sockaddr_in;

    is(typeid(*sa) == typeid(*sa_in), true, test + "expected type");

    delete sa_in;
    delete sa;
}

void test_sockaddr_in6_blank_constructor(void)
{
    std::string test = "sockaddr_in6 blank constructor: ";
    Sockaddr_in6 *sa = new Sockaddr_in6;

    is(sa->sin6->sin6_family, AF_INET6, test + "expected family");
    is(sa->sin6->sin6_port, htons(0), test + "expected port");
    is(sa->sin6->sin6_flowinfo, htonl(0L), test + "expected flowinfo");
    is(sa->sin6->sin6_scope_id, htonl(0L), test + "expected scope id");
    is(memcmp(&sa->sin6->sin6_addr, &in6addr_any, sizeof(struct in6_addr)), 0,
       test + "expected addr");

    delete sa;
}

void test_sockaddr_in6_copy_constructor(void)
{
    std::string test = "sockaddr_in6 copy constructor: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    Sockaddr_in6 *sa = new Sockaddr_in6;

    sa->sin6->sin6_port = htons(1234);
    memcpy(&sa->sin6->sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa2 = new Sockaddr_in6(*((const Sockaddr *)sa));

    is(sa2->sin6->sin6_family, AF_INET6, test + "expected family");
    is(sa2->sin6->sin6_port, htons(1234), test + "expected port");
    is(sa2->sin6->sin6_flowinfo, htonl(0L), test + "expected flowinfo");
    is(sa2->sin6->sin6_scope_id, htonl(0L), test + "expected scope id");
    is(memcmp(&sa2->sin6->sin6_addr, &ip_addr, sizeof(struct in6_addr)), 0,
       test + "expected addr");

    delete sa2;
    delete sa;
}

void test_sockaddr_in6_struct_constructor(void)
{
    std::string test = "sockaddr_in6 struct constructor: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    is(sa->sin6->sin6_family, AF_INET6, test + "expected family");
    is(sa->sin6->sin6_port, htons(1234), test + "expected port");
    is(sa->sin6->sin6_flowinfo, htonl(0L), test + "expected flowinfo");
    is(sa->sin6->sin6_scope_id, htonl(0L), test + "expected scope id");
    is(memcmp(&sa->sin6->sin6_addr, &ip_addr, sizeof(struct in6_addr)), 0,
       test + "expected addr");

    delete sa;
}

void test_sockaddr_in6_equal_comparison(void)
{
    std::string test = "sockaddr_in6 equal: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa1 = new Sockaddr_in6((const struct sockaddr&)sin);
    Sockaddr_in6 *sa2 = new Sockaddr_in6((const struct sockaddr&)sin);

    is(*sa1 == *sa2, true, test + "objects equal");

    sa2->sin6->sin6_port = htons(2345);

    is(*sa1 == *sa2, false, test + "objects not equal");

    Sockaddr_in sain;

    is(*sa1 == *((Sockaddr *)&sain), false,
       test + "object not equal to different object");

    delete sa2;
    delete sa1;
}

void test_sockaddr_in6_less_comparison(void)
{
    std::string test = "sockaddr_in6 less: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa1 = new Sockaddr_in6((const struct sockaddr&)sin);
    Sockaddr_in6 *sa2 = new Sockaddr_in6((const struct sockaddr&)sin);

    ++(sa2->sin6->sin6_addr.s6_addr[15]);

    is(*sa1 < *sa2, true, test + "object less than object");

    ++(sa1->sin6->sin6_addr.s6_addr[14]);

    is(*sa1 < *sa2, false, test + "object not less than object");
    is(*sa1 < *((const struct sockaddr *)&sin), false,
       test + "object not less than struct");

    Sockaddr_in sain;

    is(*sa1 < *((Sockaddr *)&sain), false,
       test + "object not less than different object");

    delete sa2;
    delete sa1;
}

void test_sockaddr_in6_ntop(void)
{
    std::string test = "sockaddr_in6 ntop: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    is(strncmp(sa->ntop(), v6_ADDRESS, strlen(v6_ADDRESS)), 0,
       test + "ntop equal to addr");

    delete sa;
}

void test_sockaddr_in6_hostname(void)
{
    std::string test = "sockaddr_in6 hostname: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    is(strcmp(sa->hostname(), HOST_NAME), 0, test + "hostnames equal");

    delete sa;
}

void test_sockaddr_in6_port(void)
{
    std::string test = "sockaddr_in6 port: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    is(sa->port(), 1234, test + "ports equal");

    delete sa;
}

void test_sockaddr_in6_sockaddr(void)
{
    std::string test = "sockaddr_in6: ";
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    is(ret, 1, test + "pton successful");

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)sa->sockaddr();
    is(sa->sin6->sin6_family, AF_INET6, test + "expected family");
    is(sa->sin6->sin6_port, htons(1234), test + "expected port");
    is(sa->sin6->sin6_flowinfo, htonl(0L), test + "expected flowinfo");
    is(sa->sin6->sin6_scope_id, htonl(0L), test + "expected scope id");
    is(memcmp(&sa->sin6->sin6_addr, &ip_addr, sizeof(struct in6_addr)), 0,
       test + "expected addr");

    delete sa;
}

void test_sockaddr_in6_factory(void)
{
    std::string test = "sockaddr_in6 factory: ";
    struct sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(1234);
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &sin6.sin6_addr);
    is(ret, 1, test + "pton successful");

    Sockaddr *sa = build_sockaddr((struct sockaddr &)sin6);
    Sockaddr_in6 *sa_in6 = new Sockaddr_in6;

    is(typeid(*sa) == typeid(*sa_in6), true, test + "expected type");

    delete sa_in6;
    delete sa;
}

void test_sockaddr_un_blank_constructor(void)
{
    std::string test = "sockaddr_un blank constructor: ";
    Sockaddr_un *su = new Sockaddr_un;

    is(su->sun->sun_family, AF_UNIX, test + "expected family");
    is(strcmp(su->sun->sun_path, ""), 0, test + "expected path");

    delete su;
}

void test_sockaddr_un_copy_constructor(void)
{
    std::string test = "sockaddr_un copy constructor: ";
    char path[] = "/this/is/a/path";

    Sockaddr_un *su = new Sockaddr_un;

    strcpy(su->sun->sun_path, path);

    Sockaddr_un *su2 = new Sockaddr_un(*((const Sockaddr *)su));

    is(su2->sun->sun_family, AF_UNIX, test + "expected family");
    is(strcmp(su2->sun->sun_path, path), 0, test + "expected path");

    delete su2;
    delete su;
}

void test_sockaddr_un_struct_constructor(void)
{
    std::string test = "sockaddr_un struct constructor: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    is(su->sun->sun_family, AF_UNIX, test + "expected family");
    is(strcmp(su->sun->sun_path, path), 0, test + "expected path");

    delete su;
}

void test_sockaddr_un_equal_comparison(void)
{
    std::string test = "sockaddr_un equal: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su1 = new Sockaddr_un((const struct sockaddr&)sun);
    Sockaddr_un *su2 = new Sockaddr_un((const struct sockaddr&)sun);

    is(*su1 == *su2, true, test + "objects equal");

    su2->sun->sun_path[2] = 'q';

    is(*su1 == *su2, false, test + "objects not equal");

    Sockaddr_in6 sa6;

    is(*su1 == *((Sockaddr *)&sa6), false,
       test + "object not equal to different object");

    delete su2;
    delete su1;
}

void test_sockaddr_un_less_comparison(void)
{
    std::string test = "sockaddr_un less: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su1 = new Sockaddr_un((const struct sockaddr&)sun);
    Sockaddr_un *su2 = new Sockaddr_un((const struct sockaddr&)sun);

    is(*su1 < *su2, false, test + "object not less than object");

    su2->sun->sun_path[2] = 'q';

    is(*su1 < *su2, true, test + "object less than object");

    su2->sun->sun_path[2] = 'a';

    is(*su1 < *((const struct sockaddr *)&sun), false,
       test + "object not less than struct");
    is(*su2 < *((const struct sockaddr *)&sun), true,
       test + "object less than object");

    Sockaddr_in6 sa6;

    is(*su1 < *((Sockaddr *)&sa6), false,
       test + "object not less than different object");

    delete su2;
    delete su1;
}

void test_sockaddr_un_ntop(void)
{
    std::string test = "sockaddr_un ntop: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    is(strcmp(su->ntop(), path), 0, test + "expected path");

    delete su;
}

void test_sockaddr_un_hostname(void)
{
    std::string test = "sockaddr_un hostname: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    is(strcmp(su->hostname(), "localhost"), 0, test + "expected hostname");

    delete su;
}

void test_sockaddr_un_port(void)
{
    std::string test = "sockaddr_un port: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    is(su->port(), UINT16_MAX, test + "ports equal");

    delete su;
}

void test_sockaddr_un_sockaddr(void)
{
    std::string test = "sockaddr_un: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    struct sockaddr_un *saddr = (struct sockaddr_un *)su->sockaddr();
    is(saddr->sun_family, AF_UNIX, test + "expected family");
    is(strcmp(saddr->sun_path, path), 0, test + "expected path");

    delete su;
}

void test_sockaddr_un_factory(void)
{
    std::string test = "sockaddr_un factory: ";
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr *sa = build_sockaddr((struct sockaddr&)sun);
    Sockaddr_un *su = new Sockaddr_un;

    is(typeid(*sa) == typeid(*su), true, test + "expected type");

    delete su;
    delete sa;
}

int main(int argc, char **argv)
{
    plan(104);

    test_sockaddr_blank_constructor();
    test_sockaddr_copy_constructor();
    test_sockaddr_struct_constructor();
    test_sockaddr_equal_comparison();
    test_sockaddr_assignment();
    test_sockaddr_sockaddr();
    test_sockaddr_factory();

    test_sockaddr_in_blank_constructor();
    test_sockaddr_in_copy_constructor();
    test_sockaddr_in_struct_constructor();
    test_sockaddr_in_equal_comparison();
    test_sockaddr_in_less_comparison();
    test_sockaddr_in_ntop();
    test_sockaddr_in_hostname();
    test_sockaddr_in_port();
    test_sockaddr_in_sockaddr();
    test_sockaddr_in_factory();

    test_sockaddr_in6_blank_constructor();
    test_sockaddr_in6_copy_constructor();
    test_sockaddr_in6_struct_constructor();
    test_sockaddr_in6_equal_comparison();
    test_sockaddr_in6_less_comparison();
    test_sockaddr_in6_ntop();
    test_sockaddr_in6_hostname();
    test_sockaddr_in6_port();
    test_sockaddr_in6_sockaddr();
    test_sockaddr_in6_factory();

    test_sockaddr_un_blank_constructor();
    test_sockaddr_un_copy_constructor();
    test_sockaddr_un_struct_constructor();
    test_sockaddr_un_equal_comparison();
    test_sockaddr_un_less_comparison();
    test_sockaddr_un_ntop();
    test_sockaddr_un_hostname();
    test_sockaddr_un_port();
    test_sockaddr_un_sockaddr();
    test_sockaddr_un_factory();
    return exit_status();
}
