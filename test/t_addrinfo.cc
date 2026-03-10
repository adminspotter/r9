#include <tap++.h>

using namespace TAP;

#include "../server/classes/addrinfo.h"

void test_addrinfo(void)
{
    std::string test = "addrinfo: ", st;
    Addrinfo *ai = NULL;

    st = "bad addr: ";

    try
    {
        ai = new Addrinfo(STREAM, "999.999.999.999", "1234");
    }
    catch (std::runtime_error& e)
    {
        pass(test + st + "expected error type");
    }
    catch (...)
    {
        fail(test + st + "wrong error type");
    }

    st = "stream: ";

    ai = new Addrinfo(STREAM, "1.2.3.4", "1234");

    ok(ai != NULL, test + st + "non-null result");
    ok(ai->ai != NULL, test + st + "non-null ai");
    is(ai->ai->ai_family, AF_INET, test + st + "expected address family");
    is(ai->ai->ai_socktype, SOCK_STREAM, test + st + "expected socket type");

    ok(ai->family() == ai->ai->ai_family, test + st + "good family");
    ok(ai->socktype() == ai->ai->ai_socktype, test + st + "good socktype");
    ok(ai->protocol() == ai->ai->ai_protocol, test + st + "good protocol");
    ok(ai->addrlen() == ai->ai->ai_addrlen, test + st + "good addrlen");
    ok(ai->canonname() == ai->ai->ai_canonname, test + st + "good canonname");

    delete ai;

    st = "dgram: ";

    ai = new Addrinfo(DGRAM, "1.2.3.4", "1234");

    ok(ai != NULL, test + st + "non-null result");
    ok(ai->ai != NULL, test + st + "non-null ai");
    is(ai->ai->ai_family, AF_INET, test + st + "expected address family");
    is(ai->ai->ai_socktype, SOCK_DGRAM, test + st + "expected socket type");

    delete ai;
}

void test_addrinfo_un(void)
{
    std::string test = "addrinfo_un: ";
    Addrinfo_un *au = NULL;

    au = new Addrinfo_un("abc");
    ok(au != NULL, test + "non-null result");
    ok(au->ai != NULL, test + "non-null ai");
    ok(au->ai->ai_addr != NULL, test + "non-null ai_addr");

    delete au;
}

void test_build_addrinfo(void)
{
    std::string test = "build_addrinfo: ", st;
    Addrinfo *ai = NULL;

    ai = build_addrinfo(UNIX, "abc", "123");

    ok(dynamic_cast<Addrinfo_un *>(ai) != NULL, test + "expected unix object");

    delete ai;

    ai = build_addrinfo(STREAM, "1.2.3.4", "1234");

    ok(ai != NULL, test + "non-null stream type");
    is(ai->ai->ai_socktype, SOCK_STREAM, test + st + "expected socket type");

    delete ai;

    ai = build_addrinfo(DGRAM, "1.2.3.4", "1234");

    ok(ai != NULL, test + "non-null dgram type");
    is(ai->ai->ai_socktype, SOCK_DGRAM, test + st + "expected socket type");

    delete ai;
}

void test_str_to_addrinfo(void)
{
    std::string test = "str_to_addrinfo: ";
    Addrinfo *ai = NULL;

    ai = str_to_addrinfo("broken");
    ok(ai == NULL, test + "no colon");

    ai = str_to_addrinfo("bogus:whatever");
    ok(ai == NULL, test + "bad type");

    ai = str_to_addrinfo("unix:thing");
    ok(ai != NULL, test + "good unix");
    delete ai;
    ai = NULL;

    ai = str_to_addrinfo("dgram:1.2.3.4");
    ok(ai == NULL, test + "no port");

    ai = str_to_addrinfo("dgram:1.2.3.4:9876");
    ok(ai != NULL, test + "good v4");
    delete ai;
    ai = NULL;

    ai = str_to_addrinfo("stream:9876");
    ok(ai == NULL, test + "no addr");

    ai = str_to_addrinfo("stream:f00f::abcd:9876");
    ok(ai == NULL, test + "v6 no [");

    ai = str_to_addrinfo("stream:[f00f::abcd:9876");
    ok(ai == NULL, test + "v6 no ]");

    ai = str_to_addrinfo("stream:[f00f::abcd]:9876");
    ok(ai != NULL, test + "good v6");
    delete ai;
}

int main(int argc, char **argv)
{
    plan(31);

    test_addrinfo();
    test_addrinfo_un();
    test_build_addrinfo();
    test_str_to_addrinfo();

    return exit_status();
}
