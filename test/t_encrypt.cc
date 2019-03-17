#include <tap++.h>

#include <string.h>

using namespace TAP;

#include "../proto/encrypt.h"

void test_encrypt(void)
{
    std::string test = "r9_encrypt: ";

    const unsigned char plaintext[16] = "test text";
    const unsigned char key[33] = "abcd1234efgh5678ijkl9012mnop3456";
    const unsigned char iv[33] = "12345678123456781234567812345678";
    unsigned char ciphertext[16];

    memset(ciphertext, 0, sizeof(ciphertext));
    int result = r9_encrypt(plaintext, strlen((const char *)plaintext),
                            key, iv, ciphertext);

    is(result, 9, test + "expected result");

    const unsigned char expected[16] =
        {
            0x18, 0xf0, 0x15, 0xd0, 0x81, 0x08, 0xb6, 0xca,
            0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
    is(memcmp(ciphertext, expected, sizeof(expected)),
       0,
       test + "expected ciphertext");
    isnt(memcmp(ciphertext, plaintext, sizeof(plaintext)),
         0,
         test + "ciphertext is not plaintext");
}

void test_decrypt(void)
{
    std::string test = "r9_decrypt: ";

    const unsigned char plaintext[16] = "test text";
    const unsigned char key[33] = "abcd1234efgh5678ijkl9012mnop3456";
    const unsigned char iv[33] = "12345678123456781234567812345678";
    unsigned char ciphertext[16], cleartext[16];

    memset(ciphertext, 0, sizeof(ciphertext));
    memset(cleartext, 0, sizeof(cleartext));
    int cipherlen = r9_encrypt(plaintext, strlen((const char *)plaintext),
                               key, iv, ciphertext);
    int result = r9_decrypt(ciphertext, cipherlen, key, iv, cleartext);

    is(result, 9, test + "expected result");
    is(memcmp(plaintext, cleartext, sizeof(cleartext)),
       0,
       test + "expected cleartext");
}

int main(int argc, char **argv)
{
    plan(5);

    test_encrypt();
    test_decrypt();
    return exit_status();
}
