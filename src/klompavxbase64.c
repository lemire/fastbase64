#include "klompavxbase64.h"

#include <x86intrin.h>

/**
* This code is based on Alfred Klomp's https://github.com/aklomp/base64 (published under BSD)
* with minor modifications by D. Lemire.
**/
/*
Copyright (c) 2013-2015, Alfred Klomp
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/**
* Note : Hardware such as Knights Landing might do poorly with this AVX2 code since it relies on shuffles. Alternatives might be faster.
*/

static const uint8_t base64_table_enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";


// In the lookup table below, note that the value for '=' (character 61) is
// 254, not 255. This character is used for in-band signaling of the end of
// the datastream, and we will use that later. The characters A-Z, a-z, 0-9
// and + / are mapped to their "decoded" values. The other bytes all map to
// the value 255, which flags them as "invalid input".



static
const uint8_t base64_table_dec[] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, //   0..15
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, //  16..31
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255,
    255, 63, //  32..47
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 254,
    255, 255, //  48..63
    255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,
    13,  14, //  64..79
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255,
    255, 255, //  80..95
    255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
    39,  40, //  96..111
    41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255,
    255, 255, // 112..127
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, // 128..143
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};



struct base64_state {
	int eof;
	int bytes;
	//int flags;// unused
	unsigned char carry;
};
// Cast away unused variable, silence compiler:
#define UNUSED(x) ((void)(x))

// Stub function when encoder arch unsupported:
#define BASE64_ENC_STUB                                                        \
  UNUSED(state);                                                               \
  UNUSED(src);                                                                 \
  UNUSED(srclen);                                                              \
  UNUSED(out);                                                                 \
                                                                               \
  *outlen = 0;

// Stub function when decoder arch unsupported:
#define BASE64_DEC_STUB                                                        \
  UNUSED(state);                                                               \
  UNUSED(src);                                                                 \
  UNUSED(srclen);                                                              \
  UNUSED(out);                                                                 \
  UNUSED(outlen);                                                              \
                                                                               \
  return -1;

struct codec {
  void (*enc)(struct base64_state *state, const char *src, size_t srclen,
              char *out, size_t *outlen);
  int (*dec)(struct base64_state *state, const char *src, size_t srclen,
             char *out, size_t *outlen);
};

// Define machine endianness. This is for GCC:
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define BASE64_LITTLE_ENDIAN 1
#else
#define BASE64_LITTLE_ENDIAN 0
#endif

// This is for Clang:
#ifdef __LITTLE_ENDIAN__
#define BASE64_LITTLE_ENDIAN 1
#endif

#ifdef __BIG_ENDIAN__
#define BASE64_LITTLE_ENDIAN 0
#endif

// Endian conversion functions:
#if BASE64_LITTLE_ENDIAN
#if defined(_MSC_VER)
// Microsoft Visual C++:
#define cpu_to_be32(x) _byteswap_ulong(x)
#define cpu_to_be64(x) _byteswap_uint64(x)
#define be32_to_cpu(x) _byteswap_ulong(x)
#define be64_to_cpu(x) _byteswap_uint64(x)
#else
// GCC and Clang:
#define cpu_to_be32(x) __builtin_bswap32(x)
#define cpu_to_be64(x) __builtin_bswap64(x)
#define be32_to_cpu(x) __builtin_bswap32(x)
#define be64_to_cpu(x) __builtin_bswap64(x)
#endif
#else
// No conversion needed:
#define cpu_to_be32(x) (x)
#define cpu_to_be64(x) (x)
#define be32_to_cpu(x) (x)
#define be64_to_cpu(x) (x)
#endif

// detect word size
#ifdef _INTEGRAL_MAX_BITS
#define BASE64_WORDSIZE _INTEGRAL_MAX_BITS
#else
#define BASE64_WORDSIZE __WORDSIZE
#endif

// end-of-file definitions
// Almost end-of-file when waiting for the last '=' character:
#define BASE64_AEOF 1
// End-of-file when stream end has been reached or invalid input provided:
#define BASE64_EOF 2


#define CMPGT(s, n) _mm256_cmpgt_epi8((s), _mm256_set1_epi8(n))
#define CMPEQ(s, n) _mm256_cmpeq_epi8((s), _mm256_set1_epi8(n))
#define REPLACE(s, n) _mm256_and_si256((s), _mm256_set1_epi8(n))
#define RANGE(s, a, b) _mm256_andnot_si256(CMPGT((s), (b)), CMPGT((s), (a)-1))

static inline __m256i _mm256_bswap_epi32(const __m256i in) {
  // _mm256_shuffle_epi8() works on two 128-bit lanes separately:
  return _mm256_shuffle_epi8(in, _mm256_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11,
                                                  10, 9, 8, 15, 14, 13, 12, 3,
                                                  2, 1, 0, 7, 6, 5, 4, 11, 10,
                                                  9, 8, 15, 14, 13, 12));
}

static inline __m256i enc_reshuffle(__m256i in) {
  // Spread out 32-bit words over both halves of the input register:
  in = _mm256_permutevar8x32_epi32(in,
                                   _mm256_setr_epi32(0, 1, 2, -1, 3, 4, 5, -1));

  // Slice into 32-bit chunks and operate on all chunks in parallel.
  // All processing is done within the 32-bit chunk. First, shuffle:
  // before: [eeeeeeff|ccdddddd|bbbbcccc|aaaaaabb]
  // after:  [00000000|aaaaaabb|bbbbcccc|ccdddddd]
  in = _mm256_shuffle_epi8(in,
                           _mm256_set_epi8(-1, 9, 10, 11, -1, 6, 7, 8, -1, 3, 4,
                                           5, -1, 0, 1, 2, -1, 9, 10, 11, -1, 6,
                                           7, 8, -1, 3, 4, 5, -1, 0, 1, 2));

  // merged  = [0000aaaa|aabbbbbb|bbbbcccc|ccdddddd]
  const __m256i merged = _mm256_blend_epi16(_mm256_slli_epi32(in, 4), in, 0x55);

  // bd      = [00000000|00bbbbbb|00000000|00dddddd]
  const __m256i bd = _mm256_and_si256(merged, _mm256_set1_epi32(0x003F003F));

  // ac      = [00aaaaaa|00000000|00cccccc|00000000]
  const __m256i ac = _mm256_and_si256(_mm256_slli_epi32(merged, 2),
                                      _mm256_set1_epi32(0x3F003F00));

  // indices = [00aaaaaa|00bbbbbb|00cccccc|00dddddd]
  const __m256i indices = _mm256_or_si256(ac, bd);

  // return  = [00dddddd|00cccccc|00bbbbbb|00aaaaaa]
  return _mm256_bswap_epi32(indices);
}

static inline __m256i enc_translate(const __m256i in) {
  // LUT contains Absolute offset for all ranges:
  const __m256i lut = _mm256_setr_epi8(
      65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0, 65, 71,
      -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);
  // Translate values 0..63 to the Base64 alphabet. There are five sets:
  // #  From      To         Abs    Index  Characters
  // 0  [0..25]   [65..90]   +65        0  ABCDEFGHIJKLMNOPQRSTUVWXYZ
  // 1  [26..51]  [97..122]  +71        1  abcdefghijklmnopqrstuvwxyz
  // 2  [52..61]  [48..57]    -4  [2..11]  0123456789
  // 3  [62]      [43]       -19       12  +
  // 4  [63]      [47]       -16       13  /

  // Create LUT indices from input:
  // the index for range #0 is right, others are 1 less than expected:
  __m256i indices = _mm256_subs_epu8(in, _mm256_set1_epi8(51));

  // mask is 0xFF (-1) for range #[1..4] and 0x00 for range #0:
  __m256i mask = CMPGT(in, 25);

  // substract -1, so add 1 to indices for range #[1..4], All indices are now
  // correct:
  indices = _mm256_sub_epi8(indices, mask);

  // Add offsets to input values:
  __m256i out = _mm256_add_epi8(in, _mm256_shuffle_epi8(lut, indices));

  return out;
}

static inline __m256i dec_reshuffle(__m256i in) {
  // Mask in a single byte per shift:
  const __m256i maskB2 = _mm256_set1_epi32(0x003F0000);
  const __m256i maskB1 = _mm256_set1_epi32(0x00003F00);

  // Pack bytes together:
  __m256i out = _mm256_srli_epi32(in, 16);

  out =
      _mm256_or_si256(out, _mm256_srli_epi32(_mm256_and_si256(in, maskB2), 2));

  out =
      _mm256_or_si256(out, _mm256_slli_epi32(_mm256_and_si256(in, maskB1), 12));

  out = _mm256_or_si256(out, _mm256_slli_epi32(in, 26));

  // Pack bytes together within 32-bit words, discarding words 3 and 7:
  out = _mm256_shuffle_epi8(out, _mm256_setr_epi8(3, 2, 1, 7, 6, 5, 11, 10, 9,
                                                  15, 14, 13, -1, -1, -1, -1, 3,
                                                  2, 1, 7, 6, 5, 11, 10, 9, 15,
                                                  14, 13, -1, -1, -1, -1));

  // Pack 32-bit words together, squashing empty words 3 and 7:
  return _mm256_permutevar8x32_epi32(
      out, _mm256_setr_epi32(0, 1, 2, 4, 5, 6, -1, -1));
}

static void base64_stream_encode_avx2(struct base64_state *state, const char *src,
                               size_t srclen, char *out, size_t *outlen) {
  // Assume that *out is large enough to contain the output.
  // Theoretically it should be 4/3 the length of src.
  const uint8_t *c = (const uint8_t *)src;
  uint8_t *o = (uint8_t *)out;

  // Use local temporaries to avoid cache thrashing:
  size_t outl = 0;
  struct base64_state st;
  st.bytes = state->bytes;
  st.carry = state->carry;

  // Turn three bytes into four 6-bit numbers:
  // in[0] = 00111111
  // in[1] = 00112222
  // in[2] = 00222233
  // in[3] = 00333333

  // Duff's device, a for() loop inside a switch() statement. Legal!
  switch (st.bytes) {
    for (;;) {
    case 0:
      // If we have AVX2 support, pick off 24 bytes at a time for as long as we
      // can.
      // But because we read 32 bytes at a time, ensure we have enough room to
      // do a
      // full 32-byte read without segfaulting:
      while (srclen >= 32) {
        // Load string:
        __m256i str = _mm256_loadu_si256((__m256i *)c);

        // Reshuffle:
        str = enc_reshuffle(str);

        // Translate reshuffled bytes to the Base64 alphabet:
        str = enc_translate(str);

        // Store:
        _mm256_storeu_si256((__m256i *)o, str);

        c += 24; // 6 * 4 bytes of input
        o += 32; // 8 * 4 bytes of output
        outl += 32;
        srclen -= 24;
      }
      if (srclen-- == 0) {
        break;
      }
      *o++ = base64_table_enc[*c >> 2];
      st.carry = (*c++ << 4) & 0x30;
      st.bytes++;
      outl += 1;

    case 1:
      if (srclen-- == 0) {
        break;
      }
      *o++ = base64_table_enc[st.carry | (*c >> 4)];
      st.carry = (*c++ << 2) & 0x3C;
      st.bytes++;
      outl += 1;

    case 2:
      if (srclen-- == 0) {
        break;
      }
      *o++ = base64_table_enc[st.carry | (*c >> 6)];
      *o++ = base64_table_enc[*c++ & 0x3F];
      st.bytes = 0;
      outl += 2;
    }
  }
  state->bytes = st.bytes;
  state->carry = st.carry;
  *outlen = outl;
}

static int base64_stream_decode_avx2(struct base64_state *state, const char *src,
                              size_t srclen, char *out, size_t *outlen) {
  int ret = 0;
  const uint8_t *c = (const uint8_t *)src;
  uint8_t *o = (uint8_t *)out;
  uint8_t q;

  // Use local temporaries to avoid cache thrashing:
  size_t outl = 0;
  struct base64_state st;
  st.eof = state->eof;
  st.bytes = state->bytes;
  st.carry = state->carry;

  // If we previously saw an EOF or an invalid character, bail out:
  if (st.eof) {
    *outlen = 0;
    ret = 0;
    // If there was a trailing '=' to check, check it:
    if (srclen && (st.eof == BASE64_AEOF)) {
      state->bytes = 0;
      state->eof = BASE64_EOF;
      ret = ((base64_table_dec[*c++] == 254) && (srclen == 1)) ? 1 : 0;
    }
    return ret;
  }

  // Turn four 6-bit numbers into three bytes:
  // out[0] = 11111122
  // out[1] = 22223333
  // out[2] = 33444444

  // Duff's device again:
  switch (st.bytes) {
    for (;;) {
    case 0:
      // If we have AVX2 support, pick off 32 bytes at a time for as long as we
      // can,
      // but make sure that we quit before seeing any == markers at the end of
      // the
      // string. Also, because we write 8 zeroes at the end of the output,
      // ensure
      // that there are at least 11 valid bytes of input data remaining to close
      // the
      // gap. 32 + 2 + 11 = 45 bytes:
      while (srclen >= 45) {
        // Load string:
        __m256i str = _mm256_loadu_si256((__m256i *)c);

        // The input consists of six character sets in the Base64 alphabet,
        // which we need to map back to the 6-bit values they represent.
        // There are three ranges, two singles, and then there's the rest.
        //
        //  #  From       To        Add  Characters
        //  1  [43]       [62]      +19  +
        //  2  [47]       [63]      +16  /
        //  3  [48..57]   [52..61]   +4  0..9
        //  4  [65..90]   [0..25]   -65  A..Z
        //  5  [97..122]  [26..51]  -71  a..z
        // (6) Everything else => invalid input

        const __m256i set1 = CMPEQ(str, '+');
        const __m256i set2 = CMPEQ(str, '/');
        const __m256i set3 = RANGE(str, '0', '9');
        const __m256i set4 = RANGE(str, 'A', 'Z');
        const __m256i set5 = RANGE(str, 'a', 'z');

        __m256i delta = REPLACE(set1, 19);
        delta = _mm256_or_si256(delta, REPLACE(set2, 16));
        delta = _mm256_or_si256(delta, REPLACE(set3, 4));
        delta = _mm256_or_si256(delta, REPLACE(set4, -65));
        delta = _mm256_or_si256(delta, REPLACE(set5, -71));

        // Check for invalid input: if any of the delta values are zero,
        // fall back on bytewise code to do error checking and reporting:
        if (_mm256_movemask_epi8(CMPEQ(delta, 0))) {
          break;
        }

        // Now simply add the delta values to the input:
        str = _mm256_add_epi8(str, delta);

        // Reshuffle the input to packed 12-byte output format:
        str = dec_reshuffle(str);

        // Store back:
        _mm256_storeu_si256((__m256i *)o, str);

        c += 32;
        o += 24;
        outl += 24;
        srclen -= 32;
      }
      if (srclen-- == 0) {
        ret = 1;
        break;
      }
      if ((q = base64_table_dec[*c++]) >= 254) {
        st.eof = BASE64_EOF;
        // Treat character '=' as invalid for byte 0:
        break;
      }
      st.carry = q << 2;
      st.bytes++;

    case 1:
      if (srclen-- == 0) {
        ret = 1;
        break;
      }
      if ((q = base64_table_dec[*c++]) >= 254) {
        st.eof = BASE64_EOF;
        // Treat character '=' as invalid for byte 1:
        break;
      }
      *o++ = st.carry | (q >> 4);
      st.carry = q << 4;
      st.bytes++;
      outl++;

    case 2:
      if (srclen-- == 0) {
        ret = 1;
        break;
      }
      if ((q = base64_table_dec[*c++]) >= 254) {
        st.bytes++;
        // When q == 254, the input char is '='.
        // Check if next byte is also '=':
        if (q == 254) {
          if (srclen-- != 0) {
            st.bytes = 0;
            // EOF:
            st.eof = BASE64_EOF;
            q = base64_table_dec[*c++];
            ret = ((q == 254) && (srclen == 0)) ? 1 : 0;
            break;
          } else {
            // Almost EOF
            st.eof = BASE64_AEOF;
            ret = 1;
            break;
          }
        }
        // If we get here, there was an error:
        break;
      }
      *o++ = st.carry | (q >> 2);
      st.carry = q << 6;
      st.bytes++;
      outl++;

    case 3:
      if (srclen-- == 0) {
        ret = 1;
        break;
      }
      if ((q = base64_table_dec[*c++]) >= 254) {
        st.bytes = 0;
        st.eof = BASE64_EOF;
        // When q == 254, the input char is '='. Return 1 and EOF.
        // When q == 255, the input char is invalid. Return 0 and EOF.
        ret = ((q == 254) && (srclen == 0)) ? 1 : 0;
        break;
      }
      *o++ = st.carry | q;
      st.carry = 0;
      st.bytes = 0;
      outl++;
    }
  }
  state->eof = st.eof;
  state->bytes = st.bytes;
  state->carry = st.carry;
  *outlen = outl;
  return ret;
}


// In the lookup table below, note that the value for '=' (character 61) is
// 254, not 255. This character is used for in-band signaling of the end of
// the datastream, and we will use that later. The characters A-Z, a-z, 0-9
// and + / are mapped to their "decoded" values. The other bytes all map to
// the value 255, which flags them as "invalid input".

static void base64_stream_encode_init(struct base64_state *state) {
  state->eof = 0;
  state->bytes = 0;
  state->carry = 0;
  //state->flags = flags; // useless
}

static void base64_stream_encode(struct base64_state *state, const char *src,
                          size_t srclen, char *out, size_t *outlen) {
  base64_stream_encode_avx2(state, src, srclen, out, outlen);
}

static void base64_stream_encode_final(struct base64_state *state, char *out,
                                size_t *outlen) {
  uint8_t *o = (uint8_t *)out;

  if (state->bytes == 1) {
    *o++ = base64_table_enc[state->carry];
    *o++ = '=';
    *o++ = '=';
    *outlen = 3;
    return;
  }
  if (state->bytes == 2) {
    *o++ = base64_table_enc[state->carry];
    *o++ = '=';
    *outlen = 2;
    return;
  }
  *outlen = 0;
}

static void base64_stream_decode_init(struct base64_state *state) {

  state->eof = 0;
  state->bytes = 0;
  state->carry = 0;
  //state->flags = flags; // useless
}

static int base64_stream_decode(struct base64_state *state, const char *src,
                         size_t srclen, char *out, size_t *outlen) {
  return base64_stream_decode_avx2(state, src, srclen, out, outlen);
}

void klomp_avx2_base64_encode(const char *src, size_t srclen, char *out, size_t *outlen) {
  size_t s;
  size_t t;
  struct base64_state state;

  // Init the stream reader:
  base64_stream_encode_init(&state);

  // Feed the whole string to the stream reader:
  base64_stream_encode(&state, src, srclen, out, &s);

  // Finalize the stream by writing trailer if any:
  base64_stream_encode_final(&state, out + s, &t);

  // Final output length is stream length plus tail:
  *outlen = s + t;
}

int klomp_avx2_base64_decode(const char *src, size_t srclen, char *out, size_t *outlen) {
  int ret;
  struct base64_state state;

  // Init the stream reader:
  base64_stream_decode_init(&state);

  // Feed the whole string to the stream reader:
  ret = base64_stream_decode(&state, src, srclen, out, outlen);

  // If when decoding a whole block, we're still waiting for input then fail:
  if (ret && (state.bytes == 0)) {
    return ret;
  }
  return 0;
}
