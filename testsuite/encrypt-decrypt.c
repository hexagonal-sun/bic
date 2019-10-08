#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static void rotate(uint32_t *key)
{
    *key = (*key >> 1) | (*key << 31);
}

static void enc_dec(uint32_t key, char *string)
{
    while (*string)
    {
        *string ^= key;
        rotate(&key);
        string++;
    }
}

int main()
{
    uint32_t key = 0xdeadbeef;
    char *cipher_text;
    const char *plain_text = "This a cipher test; Let's hope Bob and Alice can communicate!";

    printf("Plain text: \"%s\"\n", plain_text);
    cipher_text = strdup(plain_text);

    enc_dec(key, cipher_text);
    printf("Cipher Text: \"%s\"\n", cipher_text);

    enc_dec(key, cipher_text);
    printf("Dectypted cipher Text: \"%s\"\n", cipher_text);

    free(cipher_text);

    return 0;
}
