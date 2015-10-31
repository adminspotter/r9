#include "../server/classes/sockaddr.h"

#include <iostream>
#include <typeinfo>

#include <gtest/gtest.h>

/* Test addresses */
#define v4_ADDRESS  "192.168.252.16"
#define v6_ADDRESS  "fd4b:5fd9:9623:5617:dead:beef:abcd:1234"

/* We can't instantiate a Sockaddr object directly, since there are
 * pure-virtual functions, so we'll test the specific _in and _in6
 * objects, along with a little bit of polymorphism via the factory
 * constructor.
 */

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

    Sockaddr_in *sa2 = new Sockaddr_in(*sa);

    ASSERT_EQ(sa2->sin->sin_family, AF_INET);
    ASSERT_EQ(sa2->sin->sin_port, htons(1234));
    ASSERT_EQ(sa2->sin->sin_addr.s_addr, ip_addr);

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

TEST(SockaddrInTest, Factory)
{
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1234);
    int ret = inet_pton(AF_INET, v4_ADDRESS, &sin.sin_addr);
    ASSERT_EQ(ret, 1);

    Sockaddr *sa = build_sockaddr((struct sockaddr& )sin);
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

    Sockaddr_in6 *sa2 = new Sockaddr_in6(*sa);

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
