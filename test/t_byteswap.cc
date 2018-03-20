#include <tap++.h>

using namespace TAP;

#include "../proto/proto.h"

void test_bad_type(void)
{
    std::string test = "bad type: ";
    packet p;

    p.basic.type = TYPE_PNGPKT + 1;
    p.basic.version = 1;
    p.basic.sequence = 1234LL;

    is(hton_packet(&p, sizeof(basic_packet)), 0,
       test + "hton: expected result");
    is(ntoh_packet(&p, sizeof(basic_packet)), 0,
       test + "ntoh: expected result");
    is(packet_size(&p), 0, test + "size: expected result");
}

void test_ack_packet(void)
{
    std::string test = "ack packet: ";
    packet p;

    p.ack.type = TYPE_ACKPKT;
    p.ack.version = 1;
    p.ack.sequence = 1234LL;
    p.ack.request = 42;
    p.ack.misc[0] = 2345LL;
    p.ack.misc[1] = 3456LL;
    p.ack.misc[2] = 4567LL;
    p.ack.misc[3] = 5678LL;

    is(is_ack_packet(&p), 1, test + "is an ackpkt");

    is(ntoh_packet(&p, sizeof(ack_packet) - 1), 0,
       test + "bad size fails ntoh");
    is(ntoh_packet(&p, sizeof(ack_packet)), 1,
       test + "good size succeeds ntoh");
    is(hton_packet(&p, sizeof(ack_packet) - 1), 0,
       test + "bad size fails hton");
    is(hton_packet(&p, sizeof(ack_packet)), 1,
       test + "good size succeeds hton");
}

void test_login_request(void)
{
    std::string test = "login request: ";
    packet p;

    p.log.type = TYPE_LOGREQ;
    p.log.version = 1;
    p.log.sequence = 1234LL;

    is(is_login_request(&p), 1, test + "is a logreq");

    is(ntoh_packet(&p, sizeof(login_request) - 1), 0,
       test + "bad size fails ntoh");
    is(ntoh_packet(&p, sizeof(login_request)), 1,
       test + "good size succeeds ntoh");
    is(hton_packet(&p, sizeof(login_request) - 1), 0,
       test + "bad size fails hton");
    is(hton_packet(&p, sizeof(login_request)), 1,
       test + "good size succeeds hton");
}

void test_logout_request(void)
{
    std::string test = "logout request: ";
    packet p;

    p.log.type = TYPE_LGTREQ;
    p.log.version = 1;
    p.log.sequence = 1234LL;

    is(is_logout_request(&p), 1, test + "is a lgtreq");

    is(ntoh_packet(&p, sizeof(logout_request) - 1), 0,
       test + "bad size fails ntoh");
    is(ntoh_packet(&p, sizeof(logout_request)), 1,
       test + "good size succeeds ntoh");
    is(hton_packet(&p, sizeof(logout_request) - 1), 0,
       test + "bad size fails hton");
    is(hton_packet(&p, sizeof(logout_request)), 1,
       test + "good size succeeds hton");
}

void test_action_request(void)
{
    std::string test = "action request: ";
    packet p;

    p.act.type = TYPE_ACTREQ;
    p.act.version = 1;
    p.act.sequence = 1234LL;
    p.act.object_id = 12345LL;
    p.act.action_id = 123;
    p.act.x_pos_source = 123LL;
    p.act.y_pos_source = 234LL;
    p.act.z_pos_source = 345LL;
    p.act.dest_object_id = 12346LL;
    p.act.x_pos_dest = 456LL;
    p.act.y_pos_dest = 567LL;
    p.act.z_pos_dest = 678LL;

    is(is_action_request(&p), 1, test + "is an actreq");

    is(ntoh_packet(&p, sizeof(action_request) - 1), 0,
       test + "bad size fails ntoh");
    is(ntoh_packet(&p, sizeof(action_request)), 1,
       test + "good size succeeds ntoh");
    is(hton_packet(&p, sizeof(action_request) - 1), 0,
       test + "bad size fails hton");
    is(hton_packet(&p, sizeof(action_request)), 1,
       test + "good size succeeds hton");
}

int main(int argc, char **argv)
{
    plan(23);

    test_bad_type();
    test_ack_packet();
    test_login_request();
    test_logout_request();
    test_action_request();
    return exit_status();
}
