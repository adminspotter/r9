#include <tap++.h>

using namespace TAP;

#include "../proto/ec.h"

void test_generate_ecdh_key(void)
{
    std::string test = "generate_ecdh_key: ";

    EVP_PKEY *result = generate_ecdh_key();

    is(result != NULL, true, test + "expected result");

    OPENSSL_free(result);
}

int main(int argc, char **argv)
{
    plan(1);

    test_generate_ecdh_key();
    return exit_status();
}
