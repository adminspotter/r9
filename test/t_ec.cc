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

void test_pkey_to_public_key(void)
{
    std::string test = "pkey_to_public_key: ", st;

    EVP_PKEY *key = generate_ecdh_key();

    st = "too small buffer: ";
    uint8_t pubkey[10];

    int pub_len = pkey_to_public_key(key, pubkey, sizeof(pubkey));

    is(pub_len, 0, test + st + "expected result");

    st = "good buffer: ";
    uint8_t good_pubkey[170];

    pub_len = pkey_to_public_key(key, good_pubkey, sizeof(good_pubkey));
    is(pub_len > 0, true, test + st + "expected result");

    EVP_PKEY_free(key);
}

void test_public_key_to_pkey(void)
{
    std::string test = "public_key_to_pkey: ", st;

    EVP_PKEY *key = generate_ecdh_key();
    uint8_t pubkey[170];
    int len = pkey_to_public_key(key, pubkey, sizeof(pubkey));
    is(len > 0, true, test + "expected representation");

    EVP_PKEY *key2 = public_key_to_pkey(pubkey, len);

    is(key2 != NULL, true, test + "expected result");

    EVP_PKEY_free(key2);
    EVP_PKEY_free(key);
}

int main(int argc, char **argv)
{
    plan(5);

    test_generate_ecdh_key();
    test_pkey_to_public_key();
    test_public_key_to_pkey();
    return exit_status();
}
