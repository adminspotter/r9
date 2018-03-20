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

void test_position_update(void)
{
    std::string test = "position update: ";
    packet p;

    p.pos.type = TYPE_POSUPD;
    p.pos.version = 1;
    p.pos.sequence = 1234LL;
    p.pos.frame_number = 123;
    p.pos.x_pos = 1LL;
    p.pos.y_pos = 2LL;
    p.pos.z_pos = 3LL;
    p.pos.x_orient = 123L;
    p.pos.y_orient = 234L;
    p.pos.z_orient = 345L;
    p.pos.w_orient = 456L;
    p.pos.x_look = 4L;
    p.pos.y_look = 5L;
    p.pos.z_look = 6L;

    is(is_position_update(&p), 1, test + "is a posupd");

    is(ntoh_packet(&p, sizeof(position_update) - 1), 0,
       test + "bad size fails ntoh");
    is(ntoh_packet(&p, sizeof(position_update)), 1,
       test + "good size succeeds ntoh");
    is(hton_packet(&p, sizeof(position_update) - 1), 0,
       test + "bad size fails hton");
    is(hton_packet(&p, sizeof(position_update)), 1,
       test + "good size succeeds hton");
}

int main(int argc, char **argv)
{
    plan(28);

    test_bad_type();
    test_ack_packet();
    test_login_request();
    test_logout_request();
    test_action_request();
    test_position_update();
    return exit_status();
}
