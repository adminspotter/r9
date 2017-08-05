#include "../server/classes/dgram.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

int sendto_stage = 0;

/* recvfrom is going to drive the listen loop test in steps:
 *   1. recvfrom returns 0 length
 *   2. recvfrom sets 0 fromlen
 *   3. recvfrom sets packet which won't ntoh
 *   4. recvfrom sets a sockaddr which won't build
 *   5. recvfrom works, packet gets dispatched, sets exit flag
 */
ssize_t recvfrom(int sockfd,
                 void *buf, size_t len,
                 int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen)
{
    static int stage = 0;
    ssize_t retval;
    packet *pkt = (packet *)buf;
    struct sockaddr_in *sin = (struct sockaddr_in *)src_addr;

    std::cerr << stage << std::endl;
    switch (stage++)
    {
      case 0:
        retval = 0;
        break;

      case 1:
        retval = 1;
        *addrlen = 0;
        break;

      case 2:
        pkt->basic.type = TYPE_ACKPKT;
        retval = 1;
        break;

      case 3:
        pkt->basic.type = TYPE_ACKPKT;
        retval = sizeof(ack_packet);
        /* If we set the sockaddr all 0, it should fail to build. */
        memset(sin, 0, sizeof(struct sockaddr_in));
        break;

      default:
      case 4:
        main_loop_exit_flag = 1;
        pkt->basic.type = TYPE_ACKPKT;
        retval = sizeof(ack_packet);
        memset(sin, 0, sizeof(struct sockaddr_in));
        sin->sin_family = AF_INET;
        *addrlen = sizeof(struct sockaddr_in);
        break;
    }
    return retval;
}

ssize_t sendto(int sockfd,
               const void *buf, size_t len,
               int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
{
    if (sendto_stage++ == 0)
    {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int pthread_setcancelstate(int a, int *b)
{
    return 0;
}

int pthread_setcanceltype(int a, int *b)
{
    return 0;
}

void pthread_testcancel(void)
{
}

int ntoh_packet(packet *p, size_t s)
{
    if (p->basic.type == TYPE_ACKPKT && s == 1)
        return 0;
    return 1;
}

int hton_packet(packet *p, size_t s)
{
    static int stage = 0;

    if (stage++ == 0)
        return 0;
    return 1;
}

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

TEST(DgramSocketTest, ListenWorker)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);
    void *retval;

    main_loop_exit_flag = 0;

    ASSERT_NO_THROW(
        {
            retval = dgram_socket::dgram_listen_worker((void *)dgs);
        });
    ASSERT_TRUE(retval == NULL);

    delete dgs;
    freeaddrinfo(addr);
}

/* Send queue test
 *
 * 4 packet_list elements in the send queue:
 *   - one that won't hton
 *   - one that references a non-dgram user
 *   - one that fails to send
 *   - one that sends correctly
 */
TEST(DgramSocketTest, SendWorker)
{
    struct addrinfo *addr = create_addrinfo();
    dgram_socket *dgs = new dgram_socket(addr);
    dgram_user *dgu = new dgram_user(123LL, NULL, dgs);
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    dgu->sa = build_sockaddr((struct sockaddr&)sin);
    dgs->users[123LL] = dgu;

    base_user *bu = new base_user(124LL, NULL, NULL);
    dgs->users[124LL] = bu;

    packet_list pl;

    memset(&pl, 0, sizeof(packet_list));
    pl.buf.basic.type = TYPE_ACKPKT;

    pl.who = bu;
    dgs->send_pool->push(pl);
    dgs->send_pool->push(pl);

    pl.who = dgu;
    dgs->send_pool->push(pl);
    dgs->send_pool->push(pl);

    dgs->start();

    while (sendto_stage < 1)
        ;

    delete dgs;
    freeaddrinfo(addr);
}