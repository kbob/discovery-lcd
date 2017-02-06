#include "unicode.h"

// Encode one Unicode codepoint into a UTF-8 string.
// Returns:
//     bytes encoded on success,
//     -(bytes needed) if insufficient space,
//     INVALID_CODEPOINT if malformed string.
int encode_utf8_codepoint(uint32_t codepoint, uint8_t *encoded_out, size_t size)
{
    if (codepoint <= 0x7f) {
        if (size < 1)
            return -1;
        *encoded_out   = codepoint;
        return 1;
    } else if (codepoint <= 0x7ff) {
        if (size < 2)
            return -2;
        *encoded_out++ = 0xc0 | codepoint >>  6;
        *encoded_out   = 0x80 | codepoint       & 0x3f;
        return 2;
    } else if (codepoint <= 0xffff) {
        if (size < 3)
            return -3;
        *encoded_out++ = 0xe0 | codepoint >> 12;
        *encoded_out++ = 0x80 | codepoint >>  6 & 0x3f;
        *encoded_out   = 0x80 | codepoint       & 0x3f;
        return 3;
    } else if (codepoint <= 0x10ffff) {
        if (size < 4)
            return -4;
        *encoded_out++ = 0xf0 | codepoint >> 18;
        *encoded_out++ = 0x80 | codepoint >> 12 & 0x3f;
        *encoded_out++ = 0x80 | codepoint >>  6 & 0x3f;
        *encoded_out   = 0x80 | codepoint       & 0x3f;
        return 4;
    }
    return INVALID_CODEPOINT;
}

// Decode one Unicode codepoint from a UTF-8 string.
// Returns:
//     number of bytes consumed on success,
//     INVALID_CODEPOINT on error.
int decode_utf8_codepoint(const uint8_t *str, uint32_t *codepoint_out)
{
    uint32_t b0 = *str++;
    if (!b0) {
        // end of string
        *codepoint_out = 0;
        return 0;
    }
    if (!(b0 & 0x80)) {
        // codepoint U+0000..U+007F: one byte
        *codepoint_out = b0;
        return 1;
    }
    uint32_t b1 = *str++;
    if ((b1 & 0xc0) != 0x80)
        return INVALID_CODEPOINT;
    if ((b0 & 0xe0) == 0xC0) {
        // codepoint U+0080..U+07FF: two bytes
        *codepoint_out = (b0 & 0x1f) << 6 | (b1 & 0x3f) << 0;
        return 2;
    }
    uint32_t b2 = *str++;
    if ((b2 & 0xc0) != 0x80)
        return INVALID_CODEPOINT;
    if ((b0 & 0xf0) == 0xe0) {
        // codepoint U+0800..U+FFFF: three bytes
        *codepoint_out = (b0 & 0x0f) << 12 |
                         (b1 & 0x3f) <<  6 |
                         (b2 & 0x3f) <<  0;
        return 3;
    }
    uint32_t b3 = *str++;
    if ((b3 & 0xc0) != 0x80)
        return INVALID_CODEPOINT;
    if ((b0 & 0xf8) == 0xf0) {
        // codepoint U+10000..U+10FFFF: four bytes
        *codepoint_out = (b0 & 0x07) << 18 |
                         (b1 & 0x3f) << 12 |
                         (b1 & 0x3f) <<  6 |
                         (b2 & 0x3f) <<  0;
        return 4;
    }
    return INVALID_CODEPOINT;   // invalid first byte
}

// Decode a UTF-8 string into an array of codepoints.
// Returns:
//     number of codepoints on success,
//     INVALID_CODEPOINT if malformed string,
//     negative if out of space.
int decode_utf8_string(const uint8_t *str,
                       uint32_t *codepoints_out,
                       size_t codepoints_size)
{
    size_t count;

    for (count = 0; ; count++) {
        uint32_t codepoint;
        int bytes = decode_utf8_codepoint((uint8_t *)str, &codepoint);
        if (bytes == 0)
            break;
        if (bytes < 0)
            return bytes;
        if (count >= codepoints_size)
            return -1;
        codepoints_out[count] = codepoint;
        str += bytes;
    }
    codepoints_out[count] = 0;
    return count;
}
