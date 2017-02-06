#ifndef UNICODE_included
#define UNICODE_included

#include <stddef.h>
#include <stdint.h>

#define INVALID_CODEPOINT (-100)

// Encode one Unicode codepoint into a UTF-8 string.
// Returns:
//     bytes encoded on success,
//     -(bytes needed) if insufficient space,
//     INVALID_CODEPOINT if malformed string.
extern int encode_utf8_codepoint(uint32_t codepoint,
                                 uint8_t *encoded_out,
                                 size_t size);

// Decode one Unicode codepoint from a UTF-8 string.
// Returns:
//     number of bytes consumed on success,
//     INVALID_CODEPOINT on error.
extern int decode_utf8_codepoint(const uint8_t *str, uint32_t *codepoint_out);

// Decode a UTF-8 string into an array of codepoints.
// Returns:
//     number of codepoints on success,
//     INVALID_CODEPOINT if malformed string,
//     negative if out of space.
extern int decode_utf8_string(const uint8_t *str,
                              uint32_t *codepoints_out,
                              size_t codepoints_size);

#endif /* !UNICODE_included */
