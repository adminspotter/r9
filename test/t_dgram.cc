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

TEST(DgramUserTest, CreateDelete)
{
    Control *control = new Control(0LL, NULL);
    dgram_user *dgu;

    ASSERT_NO_THROW(
        {
            dgu = new dgram_user(control->userid, control, NULL);
        });

    delete dgu;
    delete control;
}

TEST(DgramUserTest, Assignment)
{
    Control *control1 = new Control(0LL, NULL);
    Control *control2 = new Control(123LL, NULL);
    dgram_user *dgu1, *dgu2;

    dgu1 = new dgram_user(control1->userid, control1, NULL);
    dgu2 = new dgram_user(control2->userid, control2, NULL);

    ASSERT_NE(dgu1->userid, dgu2->userid);
    ASSERT_NE(dgu1->control, dgu2->control);

    *dgu2 = *dgu1;

    ASSERT_EQ(dgu1->userid, dgu2->userid);
    ASSERT_EQ(dgu1->control, dgu2->control);

    delete dgu2;
    delete dgu1;
    delete control2;
    delete control1;
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

TEST(DgramSocketTest, DoLogin)
{
    database = new mock_DB("a", "b", "c", "d");

    EXPECT_CALL(*((mock_DB *)database), get_character_objectid(_, _))
        .WillOnce(Return(1234LL));
    EXPECT_CALL(*((mock_DB *)database), check_authorization(_, _))
        .WillOnce(Return(ACCESS_NONE));

    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);

    ASSERT_TRUE(dgs->users.size() == 0);
    ASSERT_TRUE(dgs->socks.size() == 0);

    Control *control = new Control(123LL, NULL);

    access_list al;

    memset(&al, 0, sizeof(access_list));
    al.what.login.who.dgram = NULL;
    al.buf.basic.type = TYPE_LOGREQ;
    strncpy(al.buf.log.username, "bobbo", sizeof(al.buf.log.username));
    strncpy(al.buf.log.password, "argh!", sizeof(al.buf.log.password));
    strncpy(al.buf.log.charname, "howdy", sizeof(al.buf.log.charname));

    dgs->do_login(123LL, control, al);

    ASSERT_TRUE(dgs->users.size() == 1);
    ASSERT_TRUE(dgs->socks.size() == 1);

    delete dgs;
    freeaddrinfo(addr);
    delete database;
}

TEST(DgramSocketTest, DoLogout)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);
    dgram_user *dgu = new dgram_user(123LL, NULL, dgs);
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    dgu->sa = build_sockaddr((struct sockaddr&)sin);

    dgs->socks[dgu->sa] = dgu;

    ASSERT_TRUE(dgs->socks.size() == 1);

    dgs->do_logout((base_user *)dgu);

    ASSERT_TRUE(dgs->socks.size() == 0);

    delete dgu;
    delete dgs;
    freeaddrinfo(addr);
}
