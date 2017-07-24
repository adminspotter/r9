#include "../server/classes/dgram.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

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
            dgu = new dgram_user(control->userid, control);
        });

    delete dgu;
    delete control;
}

TEST(DgramUserTest, Assignment)
{
    Control *control1 = new Control(0LL, NULL);
    Control *control2 = new Control(123LL, NULL);
    dgram_user *dgu1, *dgu2;

    dgu1 = new dgram_user(control1->userid, control1);
    dgu2 = new dgram_user(control2->userid, control2);

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
