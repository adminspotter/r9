#include "../server/classes/dgram.h"

#include <gtest/gtest.h>

#include "mock_db.h"
#include "mock_server_globals.h"

using ::testing::_;
using ::testing::Return;

struct addrinfo *create_addrinfo(void)
{
    struct addrinfo hints, *addr = NULL;
    static int port = 8765;
    char port_str[6];
    int ret;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    snprintf(port_str, sizeof(port_str), "%d", port--);
    if ((ret = getaddrinfo(NULL, port_str, &hints, &addr)) != 0)
    {
        std::cerr << gai_strerror(ret) << std::endl;
        throw std::runtime_error("getaddrinfo broke");
    }

    return addr;
}

TEST(DgramSocketTest, CreateDelete)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs;

    ASSERT_NO_THROW(
        {
            dgs = new dgram_socket(addr);
        });

    delete dgs;
    freeaddrinfo(addr);
}

TEST(DgramSocketTest, PortType)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);

    ASSERT_TRUE(dgs->port_type() == "datagram");

    delete dgs;
    freeaddrinfo(addr);
}

TEST(DgramSocketTest, ConnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);

    ASSERT_TRUE(dgs->users.size() == 0);
    ASSERT_TRUE(dgs->socks.size() == 0);
    ASSERT_TRUE(dgs->user_socks.size() == 0);

    base_user *bu = new base_user(123LL, NULL, dgs);

    access_list al;

    memset(&al, 0, sizeof(access_list));
    al.what.login.who.dgram = NULL;
    al.buf.basic.type = TYPE_LOGREQ;
    strncpy(al.buf.log.username, "bobbo", sizeof(al.buf.log.username));
    strncpy(al.buf.log.password, "argh!", sizeof(al.buf.log.password));
    strncpy(al.buf.log.charname, "howdy", sizeof(al.buf.log.charname));

    dgs->connect_user(bu, al);

    ASSERT_TRUE(dgs->users.size() == 1);
    ASSERT_TRUE(dgs->socks.size() == 1);
    ASSERT_TRUE(dgs->user_socks.size() == 1);

    delete dgs;
    freeaddrinfo(addr);
}

TEST(DgramSocketTest, DisconnectUser)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);
    base_user *bu = new base_user(123LL, NULL, dgs);
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    Sockaddr *sa = build_sockaddr((struct sockaddr&)sin);

    dgs->users[bu->userid] = bu;
    dgs->socks[sa] = bu;
    dgs->user_socks[bu->userid] = sa;

    ASSERT_TRUE(dgs->users.size() == 1);
    ASSERT_TRUE(dgs->socks.size() == 1);
    ASSERT_TRUE(dgs->user_socks.size() == 1);

    dgs->disconnect_user(bu);

    ASSERT_TRUE(dgs->users.size() == 0);
    ASSERT_TRUE(dgs->socks.size() == 0);
    ASSERT_TRUE(dgs->user_socks.size() == 0);

    delete bu;
    delete dgs;
    freeaddrinfo(addr);
}

TEST(DgramSocketTest, HandlePacketUnknown)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    Sockaddr *sa = build_sockaddr((struct sockaddr&)sin);

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_POSUPD;

    dgs->handle_packet(p, sa);

    /* Not sure what to assert here.  We're exercising the code, but
     * if there's nothing to do, there's nothing to prove.
     */

    delete dgs;
    freeaddrinfo(addr);
}

TEST(DgramSocketTest, HandlePacket)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);
    base_user *bu = new base_user(123LL, NULL, dgs);
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    Sockaddr *sa = build_sockaddr((struct sockaddr&)sin);

    dgs->users[bu->userid] = bu;
    dgs->socks[sa] = bu;
    dgs->user_socks[bu->userid] = sa;

    bu->timestamp = 0;

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_ACKPKT;

    dgs->handle_packet(p, sa);

    ASSERT_NE(bu->timestamp, 0);

    delete dgs;
    freeaddrinfo(addr);
}

TEST(DgramSocketTest, HandleLogin)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    Sockaddr *sa = build_sockaddr((struct sockaddr&)sin);

    packet p;
    memset(&p, 0, sizeof(packet));
    p.basic.type = TYPE_LOGREQ;

    ASSERT_TRUE(dgs->access_pool->queue_size() == 0);

    dgram_socket::handle_login(dgs, p, NULL, sa);

    ASSERT_TRUE(dgs->access_pool->queue_size() != 0);
    access_list al;
    memset(&al, 0, sizeof(access_list));
    dgs->access_pool->pop(&al);
    ASSERT_EQ(al.buf.basic.type, TYPE_LOGREQ);
    ASSERT_TRUE((void *)al.what.login.who.dgram != (void *)sa);

    delete al.what.login.who.dgram;
    delete sa;
    delete dgs;
    freeaddrinfo(addr);
}
