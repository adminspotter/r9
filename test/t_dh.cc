#include <tap++.h>

using namespace TAP;

#include <string.h>

#include "../proto/dh.h"
#include "../proto/ec.h"

unsigned char fake_data[] = "abcdefghijklmnopqrstuvwxyz0123456789";
unsigned char fake_digest[] =
{
    0xda, 0x75, 0x6a, 0x76, 0x11, 0xbc, 0x04, 0x74, 0xaf, 0x67, 0xbd,
    0xcf, 0xb7, 0x71, 0x3d, 0xd5, 0xd3, 0x3e, 0x81, 0x76, 0x80, 0x88,
    0xfd, 0xd5, 0x57, 0xac, 0x74, 0x6b, 0x9a, 0x2a, 0x2f, 0xc7
};

void test_dh_shared_secret(void)
{
    std::string test = "dh_shared_secret: ";
    EVP_PKEY *priv = generate_ecdh_key();
    EVP_PKEY *peer = generate_ecdh_key();

    is(priv != NULL, true, test + "generated private key");
    is(peer != NULL, true, test + "generated peer key");

    struct dh_message *msg = dh_shared_secret(priv, peer);

    is(msg != NULL, true, test + "generated shared secret");

    OPENSSL_free(msg->message);
    OPENSSL_free(msg);
    OPENSSL_free(peer);
    OPENSSL_free(priv);
}

void test_digest_message(void)
{
    std::string test = "digest_message: ";
    struct dh_message msg = { fake_data, 37 };

    struct dh_message *digest = digest_message(&msg);

    is(memcmp(digest->message, fake_digest, sizeof(digest->message_len)), 0,
       test + "expected digest");
    OPENSSL_free(digest->message);
    OPENSSL_free(digest);
}

int main(int argc, char **argv)
{
    plan(4);

    test_dh_shared_secret();
    test_digest_message();
    return exit_status();
}
