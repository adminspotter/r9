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

int main(int argc, char **argv)
{
    plan(3);

    test_bad_type();
    return exit_status();
}
