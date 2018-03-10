#include <tap++.h>

using namespace TAP;

#include "../client/comm.h"
#include "../client/object.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;

std::stringstream new_clog;

ObjectCache *obj = new ObjectCache("fake");
struct object *self_obj;

bool recvfrom_error = false, bad_sender = false, bad_packet = false;
bool bad_ntoh = false, bad_hton = false, sendto_error = false;
int recvfrom_calls, packet_type;

struct sockaddr_storage expected_sockaddr;
packet expected_packet;

void move_object(uint64_t a, uint16_t b,
                 float c, float d, float e,
                 float f, float g, float h, float i)
{
    is(c, 0.12, "move: expected x pos");
    is(d, 0.34, "move: expected y pos");
    is(e, 0.56, "move: expected z pos");
    is(f, 7.89, "move: expected x orientation");
    is(g, 1.23, "move: expected y orientation");
    is(h, 4.56, "move: expected z orientation");
    is(i, 7.89, "move: expected w orientation");
}

ssize_t recvfrom(int a,
                 void *b, size_t c,
                 int d,
                 struct sockaddr *e, socklen_t *f)
{
    /* Only respond to the first call; the second call should just "block" */
    if (recvfrom_calls++ != 0)
        sleep(10);

    if (recvfrom_error == true)
    {
        errno = ENOENT;
        return -1;
    }
    if (bad_sender == true)
        memset((void *)e, 1, *f);
    else
        memcpy((void *)e, (void *)&expected_sockaddr, *f);
    memcpy(b, (void *)&expected_packet, c);
    return c;
}

ssize_t sendto(int a,
               const void *b, size_t c,
               int d,
               const struct sockaddr *e, socklen_t f)
{
    if (sendto_error == true)
    {
        errno = ENOENT;
        return -1;
    }
    packet *p = (packet *)b;
    packet_type = (int)p->basic.type;
    return packet_size((packet *)b);
}

int socket(int a, int b, int c)
{
    return 123;
}

int ntoh_packet(packet *a, size_t b)
{
    if (bad_ntoh == true)
        return 0;
    return 1;
}

int hton_packet(packet *a, size_t b)
{
    if (bad_hton == true)
        return 0;
    return 1;
}

class mock_Comm : public Comm
{
  public:
    mock_Comm(struct addrinfo *ai) : Comm(ai) {};
    ~mock_Comm() {};

    MOCK_METHOD1(handle_pngpkt, void(packet& a));
    MOCK_METHOD1(handle_ackpkt, void(packet& a));
    MOCK_METHOD1(handle_posupd, void(packet& a));
    MOCK_METHOD1(handle_srvnot, void(packet& a));
    MOCK_METHOD1(handle_unsupported, void(packet& a));

    MOCK_METHOD2(send, void(packet *a, size_t b));
    MOCK_METHOD3(send_login, void(const std::string& a,
                                  const std::string& b,
                                  const std::string& c));
    MOCK_METHOD3(send_action_request,
                 void(uint16_t a, uint64_t b, uint8_t c));
    MOCK_METHOD3(send_action_request,
                 void(uint16_t a, glm::vec3& b, uint8_t c));
    MOCK_METHOD0(send_logout, void(void));
    MOCK_METHOD1(send_ack, void(uint8_t));
};

int send_ack_count = 0;

class fake_Comm : public Comm
{
  public:
    fake_Comm(struct addrinfo *ai) : Comm(ai) {};
    virtual ~fake_Comm() {};

    using Comm::src_object_id;

    using Comm::handle_pngpkt;
    using Comm::handle_ackpkt;
    using Comm::handle_posupd;
    using Comm::handle_srvnot;
    using Comm::handle_unsupported;

    virtual void send_ack(uint8_t a)
        {
            ++send_ack_count;
        };
};

TEST(CommSendTest, SendBadHton)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)pkt, 0, sizeof(packet));
    pkt->basic.type = TYPE_PNGPKT;
    pkt->basic.version = 1;

    bad_hton = true;
    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    comm->send(pkt, sizeof(packet));
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Error hton'ing packet"));
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommSendTest, SendBadSend)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)pkt, 0, sizeof(packet));
    pkt->basic.type = TYPE_PNGPKT;
    pkt->basic.version = 1;

    bad_hton = false;
    sendto_error = true;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    comm->send(pkt, sizeof(packet));
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("got a send error:"));
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommSendTest, SendPacket)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)pkt, 0, sizeof(packet));
    pkt->basic.type = TYPE_PNGPKT;
    pkt->basic.version = 1;

    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    comm->send(pkt, sizeof(packet));
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_EQ(packet_type, TYPE_PNGPKT);
    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommSendTest, SendLogin)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;
    std::string a("hi"), b("howdy"), c("hello");

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    comm->send_login(a, b, c);
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_EQ(packet_type, TYPE_LOGREQ);
    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommSendTest, SendAck)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET6;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    comm->send_ack(TYPE_LOGREQ);
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_EQ(packet_type, TYPE_ACKPKT);
    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommSendTest, SendActionReqObj)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    comm->send_action_request(1, 1, 1);
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_EQ(packet_type, TYPE_ACTREQ);
    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommSendTest, SendActionReqVec)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    glm::vec3 dest(1.0, 2.0, 3.0);
    comm->send_action_request(1, dest, 1);
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_EQ(packet_type, TYPE_ACTREQ);
    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommSendTest, SendLogout)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    Comm *comm = NULL;
    struct addrinfo ai;

    /* send_worker will delete this */
    packet *pkt = new packet;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    ASSERT_NO_THROW(
        {
            comm = new Comm(&ai);
            comm->start();
        });
    comm->send_logout();
    sleep(1);
    ASSERT_NO_THROW(
        {
            comm->stop();
        });
    delete comm;

    ASSERT_EQ(packet_type, TYPE_LGTREQ);
    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvBadResult)
{
    std::streambuf *old_clog_rdbuf = std::clog.rdbuf();
    std::stringstream new_clog;
    std::clog.rdbuf(new_clog.rdbuf());
    mock_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = true;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = false;
    recvfrom_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
            comm->start();
        });
    sleep(1);
    delete comm;

    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Error receiving packet:"));
    std::clog.rdbuf(old_clog_rdbuf);
}

void test_recv_bad_sender(void)
{
    std::string test = "unknown sender: ";
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = true;
    bad_packet = false;
    bad_ntoh = false;
    recvfrom_calls = 0;

    try
    {
        comm = new fake_Comm(&ai);
        comm->start();
    }
    catch (...)
    {
        fail(test + "constructor/start exception");
    }
    while (new_clog.str().size() == 0)
        ;
    delete comm;

    isnt(new_clog.str().find("Got packet from unknown sender"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
 }

void test_recv_bad_packet(void)
{
    std::string test = "unknown packet: ";
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = 123;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = true;
    bad_ntoh = false;
    recvfrom_calls = 0;

    try
    {
        comm = new fake_Comm(&ai);
        comm->start();
    }
    catch (...)
    {
        fail(test + "constructor/start exception");
    }
    while (new_clog.str().size() == 0)
        ;
    delete comm;

    isnt(new_clog.str().find("Unknown packet type"), std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_recv_no_ntoh(void)
{
    std::string test = "ntoh failure: ";
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = true;
    recvfrom_calls = 0;

    try
    {
        comm = new fake_Comm(&ai);
        comm->start();
    }
    catch (...)
    {
        fail(test + "constructor/start exception");
    }
    while (new_clog.str().size() == 0)
        ;
    delete comm;

    isnt(new_clog.str().find("Error while ntoh'ing packet"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_recv_ping(void)
{
    std::string test = "handle_pngpkt: ";
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm(&ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    comm->handle_pngpkt(expected_packet);
    is(send_ack_count, 1, test + "expected ack sent");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_recv_ack(void)
{
    std::string test = "handle_ackpkt: ", st;
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_ACKPKT;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm(&ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    st = "login response: ";

    expected_packet.ack.request = TYPE_LOGREQ;
    expected_packet.ack.misc[0] = ACCESS_NONE;
    expected_packet.ack.misc[1] = 12345;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Login response, type ACCESS_NONE"),
         std::string::npos,
         test + st + "good access: expected log entry");

    expected_packet.ack.misc[0] = 12345;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Login response, type unknown"),
         std::string::npos,
         test + st + "bad access: expected log entry");

    st = "logout response: ";

    expected_packet.ack.request = TYPE_LGTREQ;
    expected_packet.ack.misc[0] = ACCESS_NONE;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Logout response, type ACCESS_NONE"),
         std::string::npos,
         test + st + "good access: expected log entry");

    expected_packet.ack.misc[0] = 12345;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Logout response, type unknown"),
         std::string::npos,
         test + st + "bad access: expected log entry");

    st = "unknown response: ";

    expected_packet.ack.request = 123;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Got an unknown ack packet"),
         std::string::npos,
         test + st + "expected log entry");

    delete comm;

    new_clog.str(std::string());
}

void test_recv_pos_update(void)
{
    std::string test = "handle_posupd: ";
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_POSUPD;
    expected_packet.basic.version = 1;
    expected_packet.pos.x_pos = 12;
    expected_packet.pos.y_pos = 34;
    expected_packet.pos.z_pos = 56;
    expected_packet.pos.x_orient = 789;
    expected_packet.pos.y_orient = 123;
    expected_packet.pos.z_orient = 456;
    expected_packet.pos.w_orient = 789;

    try
    {
        comm = new fake_Comm(&ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    comm->handle_posupd(expected_packet);
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_recv_server_notice(void)
{
    std::string test = "handle_srvnot: ";
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_SRVNOT;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm(&ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    comm->handle_srvnot(expected_packet);
    delete comm;

    isnt(new_clog.str().find("Got a server notice"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_recv_unsupported(void)
{
    std::string test = "handle_unsupported: ";
    fake_Comm *comm = NULL;
    struct addrinfo ai;

    memset((void *)&ai, 0, sizeof(struct addrinfo));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_DGRAM;
    ai.ai_protocol = 17;
    ai.ai_addr = (struct sockaddr *)&expected_sockaddr;
    memset((void *)&expected_sockaddr, 0, sizeof(struct sockaddr_storage));
    expected_sockaddr.ss_family = AF_INET;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = 123;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm(&ai);
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    comm->handle_unsupported(expected_packet);
    delete comm;

    isnt(new_clog.str().find("Got an unexpected packet type: 123"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    plan(20);

    std::clog.rdbuf(new_clog.rdbuf());

    int gtests = RUN_ALL_TESTS();

    test_recv_bad_sender();
    test_recv_bad_packet();
    test_recv_no_ntoh();
    test_recv_ping();
    test_recv_ack();
    test_recv_pos_update();
    test_recv_server_notice();
    test_recv_unsupported();
    return gtests & exit_status();
}
