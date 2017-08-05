#include "../server/classes/dgram.h"

#include <gtest/gtest.h>

#include "mock_server_globals.h"

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

/* Send queue test
 *
 * 4 packet_list elements in the send queue:
 *   - one that won't hton
 *   - one that references a non-dgram user
 *   - one that fails to send
 *   - one that sends correctly
 */

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
