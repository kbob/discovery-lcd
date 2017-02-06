#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "unicode.h"

// A
// LATIN CAPITAL LETTER A
// Unicode: U+0041, UTF-8: 41

// ü
// LATIN SMALL LETTER U WITH DIAERESIS
// Unicode: U+00FC, UTF-8: C3 BC

// €
// EURO SIGN
// Unicode: U+20AC, UTF-8: E2 82 AC

static void show_encoding(uint32_t codepoint)
{
    uint8_t buf[4];
    int n = encode_utf8_codepoint(codepoint, buf, sizeof buf);
    if (n < 0)
        fprintf(stderr, "codepoint U+%X: error %d\n", codepoint, n);
    else {
        printf("codepoint U+%X:", codepoint);
        for (int i = 0; i < n; i++)
            printf(" %02X", buf[i]);
        printf("\n");
    }
}

static void show_decoding(char *str)
{
    for (int i = 0; ; i++) {
        uint32_t codepoint;
        int bytes = decode_utf8_codepoint((uint8_t *)str, &codepoint);
        if (bytes == 0)
            break;
        if (bytes < 0) {
            fprintf(stderr, "error decoding string\n");
            exit(1);
        }
        printf("str[%d] = U+%04X\n", i, codepoint);
        str += bytes;
    }
}

int main(int argc, char *argv[])
{
    uint32_t unistr[10];
    const size_t unisize = (&unistr)[1] - unistr;

    for (int i = 1; i < argc; i++) {
        for (size_t j = 0; argv[i][j]; j++)
            printf("argv[%d][%zu] = %#x\n", i, j, (unsigned char)argv[i][j]);
        
        show_decoding(argv[i]);
        int count = decode_utf8_string((uint8_t *)argv[i], unistr, unisize);
        if (count < 0) {
            fprintf(stderr, "string decode error\n");
            exit(1);
        }
        for (size_t j = 0; j < count; j++)
            printf("unicode[%zu] = U+%04X\n", j, unistr[j]);
        printf("\n");
    }

    show_encoding('A');
    show_encoding(0xfc);        // latin small letter u with diaresis
    show_encoding(0x20ac);      // euro sign

    return 0;
}
