#include <tap++.h>

using namespace TAP;

#include "../client/configdata.h"
#include "../client/comm.h"
#include "../client/object.h"

#include "../proto/dh.h"
#include "../proto/ec.h"

std::stringstream new_clog;

ConfigData config;
ObjectCache *obj = new ObjectCache("fake");
struct object *self_obj;

bool recvfrom_error = false, bad_sender = false, bad_packet = false;
bool bad_ntoh = false, bad_hton = false, bad_encrypt = false;
bool bad_decrypt = false, sendto_error = false;
bool bad_pubkey = false, bad_shared_secret = false;
int recvfrom_calls, packet_type;

struct sockaddr_storage expected_sockaddr;
packet expected_packet;

unsigned char dh_msg[R9_SYMMETRIC_KEY_BUF_SZ];
struct dh_message msg = { dh_msg, sizeof(dh_msg) };

void move_object(uint64_t a, uint16_t b,
                 float c, float d, float e,
                 float f, float g, float h, float i,
                 float j, float k, float l)
{
    is(c, 0.12, "move: expected x pos");
    is(d, 0.34, "move: expected y pos");
    is(e, 0.56, "move: expected z pos");
    is(f, 0.7890, "move: expected w orientation");
    is(g, 0.1234, "move: expected x orientation");
    is(h, 0.5678, "move: expected y orientation");
    is(i, 0.9012, "move: expected z orientation");
    is(j, 0.3456, "move: expected x look");
    is(k, 0.7890, "move: expected y look");
    is(l, 0.1234, "move: expected z look");
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
    if (bad_packet == true)
        ((packet *)b)->basic.type = 123;
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

int close(int a)
{
    return 0;
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

int r9_encrypt(const unsigned char *a, int b,
               const unsigned char *c, unsigned char *d,
               uint64_t e, unsigned char *f)
{
    if (bad_encrypt == true)
        return 0;
    return 1;
}

int r9_decrypt(const unsigned char *a, int b,
               const unsigned char *c, unsigned char *d,
               uint64_t e, unsigned char *f)
{
    if (bad_decrypt == true)
        return 0;
    return 1;
}

EVP_PKEY *public_key_to_pkey(uint8_t *a, size_t b)
{
    if (bad_pubkey == true)
        return NULL;
    return (EVP_PKEY *)&msg;
}

struct dh_message *dh_shared_secret(EVP_PKEY *a, EVP_PKEY *b)
{
    if (bad_shared_secret == true)
        return NULL;
    return &msg;
}

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
void CRYPTO_free(void *a, const char *b, int c)
#else
void CRYPTO_free(void *a)
#endif
{
    /* Do nothing. */
}

void free_dh_message(struct dh_message *a)
{
    /* Do nothing. */
}

class fake_Comm : public Comm
{
  public:
    fake_Comm(void) : Comm() {};
    fake_Comm(struct addrinfo *ai) : Comm(ai) {};
    virtual ~fake_Comm() {};

    using Comm::send_queue;
    using Comm::src_object_id;
    using Comm::key;
    using Comm::iv;

    using Comm::handle_pngpkt;
    using Comm::handle_ackpkt;
    using Comm::handle_posupd;
    using Comm::handle_srvnot;
    using Comm::handle_srvkey;
    using Comm::handle_unsupported;
};

void test_send_bad_hton(void)
{
    std::string test = "hton failure: ";
    fake_Comm *comm = NULL;

    packet *pkt = new packet;

    memset((void *)pkt, 0, sizeof(packet));
    pkt->basic.type = TYPE_PNGPKT;
    pkt->basic.version = 1;

    bad_hton = true;
    bad_encrypt = false;
    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    try
    {
        comm = new fake_Comm();
        comm->start();
    }
    catch (...)
    {
        fail(test + "constructor/start exception");
    }
    comm->send(pkt, sizeof(packet));
    while (comm->send_queue.size() != 0)
        ;
    delete comm;

    isnt(new_clog.str().find("Error hton'ing packet"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_send_bad_encrypt(void)
{
    std::string test = "encrypt failure: ";
    fake_Comm *comm = NULL;

    packet *pkt = new packet;

    memset((void *)pkt, 0, sizeof(packet));
    pkt->basic.type = TYPE_ACTREQ;
    pkt->basic.version = 1;

    bad_hton = false;
    bad_encrypt = true;
    sendto_error = false;
    recvfrom_calls = 1;
    packet_type = -1;

    try
    {
        comm = new fake_Comm();
        comm->start();
    }
    catch (...)
    {
        fail(test + "constructor/start exception");
    }
    comm->send(pkt, sizeof(packet));
    while (comm->send_queue.size() != 0)
        ;
    delete comm;

    isnt(new_clog.str().find("Error encrypting packet"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_send_bad_send(void)
{
    std::string test = "sendto failure: ";
    fake_Comm *comm = NULL;

    packet *pkt = new packet;

    memset((void *)pkt, 0, sizeof(packet));
    pkt->basic.type = TYPE_PNGPKT;
    pkt->basic.version = 1;

    bad_hton = false;
    bad_encrypt = false;
    sendto_error = true;
    recvfrom_calls = 1;
    packet_type = -1;

    try
    {
        comm = new fake_Comm();
        comm->start();
    }
    catch (...)
    {
        fail(test + "constructor/start exception");
    }
    comm->send(pkt, sizeof(packet));
    while (comm->send_queue.size() != 0)
        ;
    delete comm;

    isnt(new_clog.str().find("got a send error:"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_send_packet(void)
{
    std::string test = "send: ";
    fake_Comm *comm = NULL;

    packet *pkt = new packet;

    memset((void *)pkt, 0, sizeof(packet));
    pkt->basic.type = TYPE_PNGPKT;
    pkt->basic.version = 1;

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(comm->send_queue.size(), 0, test + "queue empty before call");
    comm->send(pkt, sizeof(packet));
    is(comm->send_queue.size(), 1, test + "expected queue entry");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_send_login(void)
{
    std::string test = "send_login: ";
    fake_Comm *comm = NULL;
    std::string a("hi"), b("howdy");

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(comm->send_queue.size(), 0, test + "queue empty before call");
    comm->send_login(a, b);
    is(comm->send_queue.size(), 1, test + "expected queue entry");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_send_ack(void)
{
    std::string test = "send_ack: ";
    fake_Comm *comm = NULL;

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(comm->send_queue.size(), 0, test + "queue empty before call");
    comm->send_ack(TYPE_LOGREQ);
    is(comm->send_queue.size(), 1, test + "expected queue entry");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_send_action_req_obj(void)
{
    std::string test = "send_action_request (objid): ";
    fake_Comm *comm = NULL;

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(comm->send_queue.size(), 0, test + "queue empty before call");
    comm->send_action_request(1, 1, 1);
    is(comm->send_queue.size(), 1, test + "expected queue entry");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_send_action_req_vec(void)
{
    std::string test = "send_action_request (vector): ";
    fake_Comm *comm = NULL;

    /* send_worker will delete this */
    packet *pkt = new packet;

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(comm->send_queue.size(), 0, test + "queue empty before call");
    glm::vec3 dest(1.0, 2.0, 3.0);
    comm->send_action_request(1, dest, 1);
    is(comm->send_queue.size(), 1, test + "expected queue entry");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_send_logout(void)
{
    std::string test = "send_logout: ";
    fake_Comm *comm = NULL;

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    is(comm->send_queue.size(), 0, test + "queue empty before call");
    comm->send_logout();
    is(comm->send_queue.size(), 1, test + "expected queue entry");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_recv_bad_result(void)
{
    std::string test = "recvfrom failure: ";
    fake_Comm *comm = NULL;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = true;
    bad_sender = false;
    bad_packet = false;
    bad_ntoh = false;
    recvfrom_calls = 0;

    try
    {
        comm = new fake_Comm();
        comm->start();
    }
    catch (...)
    {
        fail(test + "constructor/start exception");
    }
    while (new_clog.str().size() == 0)
        ;
    delete comm;

    isnt(new_clog.str().find("Error receiving packet:"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_recv_bad_sender(void)
{
    std::string test = "unknown sender: ";
    fake_Comm *comm = NULL;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = true;
    bad_packet = false;
    bad_decrypt = false;
    bad_ntoh = false;
    recvfrom_calls = 0;

    try
    {
        comm = new fake_Comm();
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
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = true;
    bad_decrypt = false;
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

void test_recv_no_decrypt(void)
{
    std::string test = "decrypt failure: ";
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

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_decrypt = true;
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

    isnt(new_clog.str().find("Error while decrypting packet"),
         std::string::npos,
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
    expected_packet.basic.type = TYPE_ACKPKT;
    expected_packet.basic.version = 1;

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_decrypt = false;
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

void test_recv_packet(void)
{
    std::string test = "recv_worker: ";
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

    recvfrom_error = false;
    bad_sender = false;
    bad_packet = false;
    bad_decrypt = false;
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

    isnt(new_clog.str().find("Got a server notice"),
         std::string::npos,
         test + "expected log entry");
    new_clog.str(std::string());
}

void test_recv_ping(void)
{
    std::string test = "handle_pngpkt: ";
    fake_Comm *comm = NULL;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_PNGPKT;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }
    comm->handle_pngpkt(expected_packet);
    is(comm->send_queue.size(), 1, test + "expected queue entry");
    delete comm;

    is(new_clog.str().size(), 0, test + "no log entry");
}

void test_recv_ack(void)
{
    std::string test = "handle_ackpkt: ", st;
    fake_Comm *comm = NULL;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_ACKPKT;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm();
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
    isnt(new_clog.str().find("Login response, access ACCESS_NONE"),
         std::string::npos,
         test + st + "good access: expected log entry");

    expected_packet.ack.misc[0] = 12345;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Login response, access unknown"),
         std::string::npos,
         test + st + "bad access: expected log entry");

    st = "logout response: ";

    expected_packet.ack.request = TYPE_LGTREQ;
    expected_packet.ack.misc[0] = ACCESS_NONE;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Logout response, access ACCESS_NONE"),
         std::string::npos,
         test + st + "good access: expected log entry");

    expected_packet.ack.misc[0] = 12345;

    comm->handle_ackpkt(expected_packet);
    isnt(new_clog.str().find("Logout response, access unknown"),
         std::string::npos,
         test + st + "bad access: expected log entry");

    st = "actreq response: ";

    expected_packet.ack.request = TYPE_ACTREQ;
    expected_packet.ack.misc[0] = ACCESS_NONE;

    int log_len = new_clog.str().size();

    comm->handle_ackpkt(expected_packet);
    is(new_clog.str().size(), log_len, test + st + "no new log entries");

    st = "unknown response: ";

    expected_packet.ack.request = 123;

    comm->handle_ackpkt(expected_packet);
    is(new_clog.str().size(), log_len, test + st + "no new log entries");

    delete comm;

    new_clog.str(std::string());
}

void test_recv_pos_update(void)
{
    std::string test = "handle_posupd: ";
    fake_Comm *comm = NULL;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_POSUPD;
    expected_packet.basic.version = 1;
    expected_packet.pos.x_pos = 12;
    expected_packet.pos.y_pos = 34;
    expected_packet.pos.z_pos = 56;
    expected_packet.pos.w_orient = 7890;
    expected_packet.pos.x_orient = 1234;
    expected_packet.pos.y_orient = 5678;
    expected_packet.pos.z_orient = 9012;
    expected_packet.pos.x_look = 3456;
    expected_packet.pos.y_look = 7890;
    expected_packet.pos.z_look = 1234;

    try
    {
        comm = new fake_Comm();
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

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_SRVNOT;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm();
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

void test_recv_server_key(void)
{
    std::string test = "handle_srvkey: ", st;
    fake_Comm *comm = NULL;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = TYPE_SRVKEY;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm();
    }
    catch (...)
    {
        fail(test + "constructor exception");
    }

    st = "bad pubkey: ";
    bad_pubkey = true;
    bad_shared_secret = false;

    comm->handle_srvkey(expected_packet);

    isnt(new_clog.str().find("Got a bad public key"),
         std::string::npos,
         test + st + "expected log entry");
    new_clog.str(std::string());

    st = "bad shared secret: ";
    bad_pubkey = false;
    bad_shared_secret = true;

    comm->handle_srvkey(expected_packet);

    isnt(new_clog.str().find("Could not derive shared secret"),
         std::string::npos,
         test + st + "expected log entry");
    new_clog.str(std::string());

    st = "good srvkey: ";
    bad_pubkey = false;
    bad_shared_secret = false;

    comm->handle_srvkey(expected_packet);

    is(new_clog.str().size(), 0, test + st + "no log entry");

    delete comm;
}

void test_recv_unsupported(void)
{
    std::string test = "handle_unsupported: ";
    fake_Comm *comm = NULL;

    memset((void *)&expected_packet, 0, sizeof(packet));
    expected_packet.basic.type = 123;
    expected_packet.basic.version = 1;

    try
    {
        comm = new fake_Comm();
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

int main(int argc, char **argv)
{
    plan(51);

    std::clog.rdbuf(new_clog.rdbuf());

    test_send_bad_hton();
    test_send_bad_encrypt();
    test_send_bad_send();
    test_send_packet();
    test_send_login();
    test_send_ack();
    test_send_action_req_obj();
    test_send_action_req_vec();
    test_send_logout();
    test_recv_bad_result();
    test_recv_bad_sender();
    test_recv_bad_packet();
    test_recv_no_decrypt();
    test_recv_no_ntoh();
    test_recv_packet();
    test_recv_ping();
    test_recv_ack();
    test_recv_pos_update();
    test_recv_server_notice();
    test_recv_server_key();
    test_recv_unsupported();
    return exit_status();
}
