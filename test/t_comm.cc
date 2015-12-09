#include "../client/comm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;

bool recvfrom_error = false, bad_sender = false, bad_packet = false;
bool bad_ntoh = false, delay_recvfrom = false;
int sockaddr_calls;

struct sockaddr_storage expected_sockaddr;
packet expected_packet;

void move_object(uint64_t a, uint16_t b,
                 double c, double d, double e,
                 double f, double g, double h)
{
    return;
}

ssize_t recvfrom(int a,
                 void *b, size_t c,
                 int d,
                 struct sockaddr *e, socklen_t *f)
{
    /* Only respond to the first call; the second call should just "block" */
    if (sockaddr_calls++ != 0)
        sleep(10);

    /* We may need to "block" for enough time for us to set our mocks */
    if (delay_recvfrom == true)
        sleep(1);

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
    MOCK_METHOD0(send_logout, void(void));
    MOCK_METHOD1(send_ack, void(uint8_t));
};

TEST(CommSendTest, SendPacket)
{
}

TEST(CommSendTest, SendLogin)
{
}

TEST(CommSendTest, SendActionReq)
{
}

TEST(CommSendTest, SendLogout)
{
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
    delay_recvfrom = false;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    sleep(1);
    delete comm;

    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Error receiving packet:"));
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvBadSender)
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

    recvfrom_error = false;
    bad_sender = true;
    bad_packet = false;
    bad_ntoh = false;
    delay_recvfrom = false;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    sleep(1);
    delete comm;

    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Got packet from unknown sender"));
    std::clog.rdbuf(old_clog_rdbuf);
 }

TEST(CommRecvTest, RecvBadPacket)
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
    expected_packet.basic.type = 123;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = true;
    bad_ntoh = false;
    delay_recvfrom = false;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    sleep(1);
    delete comm;

    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Unknown packet type"));
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvNoNtoh)
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

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = true;
    delay_recvfrom = false;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    sleep(1);
    delete comm;

    ASSERT_THAT(new_clog.str(),
                testing::HasSubstr("Error while ntoh'ing packet"));
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvPing)
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

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = false;
    delay_recvfrom = true;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    EXPECT_CALL(*comm, handle_pngpkt(_)).Times(1);
    sleep(1);
    delete comm;

    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvAck)
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
    expected_packet.basic.type = TYPE_ACKPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = false;
    delay_recvfrom = true;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    EXPECT_CALL(*comm, handle_ackpkt(_)).Times(1);
    sleep(1);
    delete comm;

    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvPosUpdate)
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
    expected_packet.basic.type = TYPE_POSUPD;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = false;
    delay_recvfrom = true;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    EXPECT_CALL(*comm, handle_posupd(_)).Times(1);
    sleep(1);
    delete comm;

    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvServerNotice)
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
    expected_packet.basic.type = TYPE_SRVNOT;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = false;
    delay_recvfrom = true;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    EXPECT_CALL(*comm, handle_srvnot(_)).Times(1);
    sleep(1);
    delete comm;

    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}

TEST(CommRecvTest, RecvUnsupported)
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
    expected_packet.basic.type = TYPE_LOGREQ;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = false;
    delay_recvfrom = true;
    sockaddr_calls = 0;

    ASSERT_NO_THROW(
        {
            comm = new mock_Comm(&ai);
        });
    EXPECT_CALL(*comm, handle_unsupported(_)).Times(1);
    sleep(1);
    delete comm;

    ASSERT_STREQ(new_clog.str().c_str(), "");
    std::clog.rdbuf(old_clog_rdbuf);
}
