#include "../server/classes/sockaddr.h"

#include <config.h>

#include <iostream>
#include <typeinfo>

#include <gtest/gtest.h>

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

TEST(SockaddrTest, BlankConstructor)
{
    fake_Sockaddr *fs = new fake_Sockaddr;

    ASSERT_EQ(fs->ss.ss_family, AF_INET);

    delete fs;
}

TEST(SockaddrTest, CopyConstructor)
{
    fake_Sockaddr *fs = new fake_Sockaddr;

    fs->ss.ss_family = 42;

    fake_Sockaddr *fs2 = new fake_Sockaddr(*(Sockaddr *)fs);

    ASSERT_EQ(fs2->ss.ss_family, 42);

    delete fs2;
    delete fs;
}

TEST(SockaddrTest, StructConstructor)
{
    struct sockaddr_in sa;

    sa.sin_family = 19;

    fake_Sockaddr *fs = new fake_Sockaddr((const struct sockaddr&)sa);

    ASSERT_EQ(fs->ss.ss_family, 19);

    delete fs;
}

TEST(SockaddrTest, EqualComparison)
{
    fake_Sockaddr *fs = new fake_Sockaddr;
    memset(&(fs->ss), 4, sizeof(struct sockaddr_storage));

    fake_Sockaddr *fs2 = new fake_Sockaddr;
    memset(&(fs2->ss), 5, sizeof(struct sockaddr_storage));

    ASSERT_FALSE(*fs == *fs2);

    memset(&(fs2->ss), 4, sizeof(struct sockaddr_storage));

    ASSERT_TRUE(*fs == *fs2);
    ASSERT_TRUE(*fs == ((struct sockaddr *)&fs2->ss));

    memset(&(fs2->ss), 0, sizeof(struct sockaddr_storage));
    memset(&(fs->ss), 0, sizeof(struct sockaddr_storage));
    delete fs2;
    delete fs;
}

TEST(SockaddrTest, Sockaddr)
{
    struct sockaddr_storage ss;
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    ss.ss_family = 33;

    fake_Sockaddr *sa = new fake_Sockaddr((const struct sockaddr&)ss);

    struct sockaddr_storage *saddr = (struct sockaddr_storage *)sa->sockaddr();
    ASSERT_EQ(saddr->ss_family, 33);

    delete sa;
}

TEST(SockaddrTest, Factory)
{
    struct sockaddr sa;

    sa.sa_family = 0;
    while (sa.sa_family == AF_INET || sa.sa_family == AF_INET6
           || sa.sa_family == AF_UNIX)
        ++sa.sa_family;

    ASSERT_THROW(
        {
            Sockaddr *s = build_sockaddr((struct sockaddr&)sa);
        },
        std::runtime_error);
}

TEST(SockaddrInTest, BlankConstructor)
{
    Sockaddr_in *sa = new Sockaddr_in;

    ASSERT_EQ(sa->sin->sin_family, AF_INET);
    ASSERT_EQ(sa->sin->sin_port, htons(0));
    ASSERT_EQ(sa->sin->sin_addr.s_addr, htonl(INADDR_ANY));

    delete sa;
}

TEST(SockaddrInTest, CopyConstructor)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    Sockaddr_in *sa = new Sockaddr_in;

    sa->sin->sin_port = htons(1234);
    sa->sin->sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa2 = new Sockaddr_in(*((Sockaddr *)sa));

    ASSERT_EQ(sa2->sin->sin_family, AF_INET);
    ASSERT_EQ(sa2->sin->sin_port, htons(1234));
    ASSERT_EQ(sa2->sin->sin_addr.s_addr, ip_addr);

    delete sa2;
    delete sa;
}

TEST(SockaddrInTest, StructConstructor)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    ASSERT_EQ(sa->sin->sin_family, AF_INET);
    ASSERT_EQ(sa->sin->sin_port, htons(1234));
    ASSERT_EQ(sa->sin->sin_addr.s_addr, ip_addr);

    delete sa;
}

TEST(SockaddrInTest, EqualComparison)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa1 = new Sockaddr_in((const struct sockaddr&)sin);
    Sockaddr_in *sa2 = new Sockaddr_in((const struct sockaddr&)sin);

    ASSERT_TRUE(*sa1 == *sa2);

    sa2->sin->sin_port = htons(2345);

    ASSERT_FALSE(*sa1 == *sa2);

    ASSERT_TRUE(*sa1 == (const struct sockaddr *)&sin);
    ASSERT_FALSE(*sa2 == (const struct sockaddr *)&sin);

    Sockaddr_in6 sa6;

    ASSERT_FALSE(*sa1 == *((Sockaddr *)&sa6));

    delete sa2;
    delete sa1;
}

TEST(SockaddrInTest, LessComparison)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa1 = new Sockaddr_in((const struct sockaddr&)sin);
    Sockaddr_in *sa2 = new Sockaddr_in((const struct sockaddr&)sin);

    sa2->sin->sin_addr.s_addr += htonl(1L);

    ASSERT_TRUE(*sa1 < *sa2);

    sa1->sin->sin_addr.s_addr += htonl(10L);

    ASSERT_FALSE(*sa1 < *sa2);
    ASSERT_FALSE(*sa1 < *((const struct sockaddr *)&sin));

    Sockaddr_in6 sa6;

    ASSERT_FALSE(*sa1 < *((Sockaddr *)&sa6));

    delete sa2;
    delete sa1;
}

TEST(SockaddrInTest, Ntop)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    ASSERT_STREQ(sa->ntop(), v4_ADDRESS);

    delete sa;
}

TEST(SockaddrInTest, Hostname)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    ASSERT_FALSE(strcmp(sa->hostname(), HOST_NAME));

    delete sa;
}

TEST(SockaddrInTest, Port)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    ASSERT_EQ(sa->port(), 1234);

    delete sa;
}

TEST(SockaddrInTest, Sockaddr)
{
    uint32_t ip_addr;
    int ret = inet_pton(AF_INET, v4_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    sin.sin_addr.s_addr = ip_addr;

    Sockaddr_in *sa = new Sockaddr_in((const struct sockaddr&)sin);

    struct sockaddr_in *saddr = (struct sockaddr_in *)sa->sockaddr();
    ASSERT_EQ(saddr->sin_family, AF_INET);
    ASSERT_EQ(saddr->sin_port, htons(1234));
    ASSERT_EQ(saddr->sin_addr.s_addr, ip_addr);

    delete sa;
}

TEST(SockaddrInTest, Factory)
{
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    int ret = inet_pton(AF_INET, v4_ADDRESS, &sin.sin_addr);
    ASSERT_EQ(ret, 1);

    Sockaddr *sa = build_sockaddr((struct sockaddr&)sin);
    Sockaddr_in *sa_in = new Sockaddr_in;

    ASSERT_EQ(typeid(*sa), typeid(*sa_in));

    delete sa_in;
    delete sa;
}

TEST(SockaddrIn6Test, BlankConstructor)
{
    Sockaddr_in6 *sa = new Sockaddr_in6;

    ASSERT_EQ(sa->sin6->sin6_family, AF_INET6);
    ASSERT_EQ(sa->sin6->sin6_port, htons(0));
    ASSERT_EQ(sa->sin6->sin6_flowinfo, htonl(0L));
    ASSERT_EQ(sa->sin6->sin6_scope_id, htonl(0L));
    ASSERT_EQ(sa->sin6->sin6_addr, in6addr_any);

    delete sa;
}

TEST(SockaddrIn6Test, CopyConstructor)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    Sockaddr_in6 *sa = new Sockaddr_in6;

    sa->sin6->sin6_port = htons(1234);
    memcpy(&sa->sin6->sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa2 = new Sockaddr_in6(*((const Sockaddr *)sa));

    ASSERT_EQ(sa2->sin6->sin6_family, AF_INET6);
    ASSERT_EQ(sa2->sin6->sin6_port, htons(1234));
    ASSERT_EQ(sa2->sin6->sin6_flowinfo, htonl(0L));
    ASSERT_EQ(sa2->sin6->sin6_scope_id, htonl(0L));
    ASSERT_EQ(sa2->sin6->sin6_addr, ip_addr);

    delete sa2;
    delete sa;
}

TEST(SockaddrIn6Test, StructConstructor)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    ASSERT_EQ(sa->sin6->sin6_family, AF_INET6);
    ASSERT_EQ(sa->sin6->sin6_port, htons(1234));
    ASSERT_EQ(sa->sin6->sin6_flowinfo, htonl(0L));
    ASSERT_EQ(sa->sin6->sin6_scope_id, htonl(0L));
    ASSERT_EQ(sa->sin6->sin6_addr, ip_addr);

    delete sa;
}

TEST(SockaddrIn6Test, EqualComparison)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa1 = new Sockaddr_in6((const struct sockaddr&)sin);
    Sockaddr_in6 *sa2 = new Sockaddr_in6((const struct sockaddr&)sin);

    ASSERT_TRUE(*sa1 == *sa2);

    sa2->sin6->sin6_port = htons(2345);

    ASSERT_FALSE(*sa1 == *sa2);

    Sockaddr_in sain;

    ASSERT_FALSE(*sa1 == *((Sockaddr *)&sain));

    delete sa2;
    delete sa1;
}

TEST(SockaddrIn6Test, LessComparison)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa1 = new Sockaddr_in6((const struct sockaddr&)sin);
    Sockaddr_in6 *sa2 = new Sockaddr_in6((const struct sockaddr&)sin);

    ++(sa2->sin6->sin6_addr.s6_addr[15]);

    ASSERT_TRUE(*sa1 < *sa2);

    ++(sa1->sin6->sin6_addr.s6_addr[14]);

    ASSERT_FALSE(*sa1 < *sa2);
    ASSERT_FALSE(*sa1 < *((const struct sockaddr *)&sin));

    Sockaddr_in sain;

    ASSERT_FALSE(*sa1 < *((Sockaddr *)&sain));

    delete sa2;
    delete sa1;
}

TEST(SockaddrIn6Test, Ntop)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    ASSERT_STREQ(sa->ntop(), v6_ADDRESS);

    delete sa;
}

TEST(SockaddrIn6Test, Hostname)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    ASSERT_FALSE(strcmp(sa->hostname(), HOST_NAME));

    delete sa;
}

TEST(SockaddrIn6Test, Port)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    ASSERT_EQ(sa->port(), 1234);

    delete sa;
}

TEST(SockaddrIn6Test, Sockaddr)
{
    struct in6_addr ip_addr;
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &ip_addr);
    ASSERT_EQ(ret, 1);

    struct sockaddr_in6 sin;
    sin.sin6_family = AF_INET6;
    sin.sin6_port = htons(1234);
    sin.sin6_flowinfo = htonl(0L);
    sin.sin6_scope_id = htonl(0L);
    memcpy(&sin.sin6_addr, &ip_addr, sizeof(struct in6_addr));

    Sockaddr_in6 *sa = new Sockaddr_in6((const struct sockaddr&)sin);

    struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)sa->sockaddr();
    ASSERT_EQ(sa->sin6->sin6_family, AF_INET6);
    ASSERT_EQ(sa->sin6->sin6_port, htons(1234));
    ASSERT_EQ(sa->sin6->sin6_flowinfo, htonl(0L));
    ASSERT_EQ(sa->sin6->sin6_scope_id, htonl(0L));
    ASSERT_EQ(sa->sin6->sin6_addr, ip_addr);

    delete sa;
}

TEST(SockaddrIn6Test, Factory)
{
    struct sockaddr_in6 sin6;
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(1234);
    int ret = inet_pton(AF_INET6, v6_ADDRESS, &sin6.sin6_addr);
    ASSERT_EQ(ret, 1);

    Sockaddr *sa = build_sockaddr((struct sockaddr &)sin6);
    Sockaddr_in6 *sa_in6 = new Sockaddr_in6;

    ASSERT_EQ(typeid(*sa), typeid(*sa_in6));

    delete sa_in6;
    delete sa;
}

TEST(SockaddrUnTest, BlankConstructor)
{
    Sockaddr_un *su = new Sockaddr_un;

    ASSERT_EQ(su->sun->sun_family, AF_UNIX);
    ASSERT_STREQ(su->sun->sun_path, "");

    delete su;
}

TEST(SockaddrUnTest, CopyConstructor)
{
    char path[] = "/this/is/a/path";

    Sockaddr_un *su = new Sockaddr_un;

    strcpy(su->sun->sun_path, path);

    Sockaddr_un *su2 = new Sockaddr_un(*((const Sockaddr *)su));

    ASSERT_EQ(su2->sun->sun_family, AF_UNIX);
    ASSERT_STREQ(su2->sun->sun_path, path);

    delete su2;
    delete su;
}

TEST(SockaddrUnTest, StructConstructor)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    ASSERT_EQ(su->sun->sun_family, AF_UNIX);
    ASSERT_STREQ(su->sun->sun_path, path);

    delete su;
}

TEST(SockaddrUnTest, EqualComparison)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su1 = new Sockaddr_un((const struct sockaddr&)sun);
    Sockaddr_un *su2 = new Sockaddr_un((const struct sockaddr&)sun);

    ASSERT_TRUE(*su1 == *su2);

    su2->sun->sun_path[2] = 'q';

    ASSERT_FALSE(*su1 == *su2);

    Sockaddr_in6 sa6;

    ASSERT_FALSE(*su1 == *((Sockaddr *)&sa6));

    delete su2;
    delete su1;
}

TEST(SockaddrUnTest, LessComparison)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su1 = new Sockaddr_un((const struct sockaddr&)sun);
    Sockaddr_un *su2 = new Sockaddr_un((const struct sockaddr&)sun);

    ASSERT_FALSE(*su1 < *su2);

    su2->sun->sun_path[2] = 'q';

    ASSERT_TRUE(*su1 < *su2);

    su2->sun->sun_path[2] = 'a';

    ASSERT_FALSE(*su1 < *((const struct sockaddr *)&sun));
    ASSERT_TRUE(*su2 < *((const struct sockaddr *)&sun));

    Sockaddr_in6 sa6;

    ASSERT_FALSE(*su1 < *((Sockaddr *)&sa6));

    delete su2;
    delete su1;
}

TEST(SockaddrUnTest, Ntop)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    ASSERT_STREQ(su->ntop(), path);

    delete su;
}

TEST(SockaddrUnTest, Hostname)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    ASSERT_STREQ(su->hostname(), "localhost");

    delete su;
}

TEST(SockaddrUnTest, Port)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    ASSERT_EQ(su->port(), UINT16_MAX);

    delete su;
}

TEST(SockaddrUnTest, Sockaddr)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr_un *su = new Sockaddr_un((const struct sockaddr&)sun);

    struct sockaddr_un *saddr = (struct sockaddr_un *)su->sockaddr();
    ASSERT_EQ(saddr->sun_family, AF_UNIX);
    ASSERT_STREQ(saddr->sun_path, path);

    delete su;
}

TEST(SockaddrUnTest, Factory)
{
    char path[] = "/this/is/a/path";

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, path);

    Sockaddr *sa = build_sockaddr((struct sockaddr&)sun);
    Sockaddr_un *su = new Sockaddr_un;

    ASSERT_EQ(typeid(*sa), typeid(*su));

    delete su;
    delete sa;
}
