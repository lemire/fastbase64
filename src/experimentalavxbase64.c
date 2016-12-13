#include "experimentalavxbase64.h"

#include <x86intrin.h>

/**
* This code borrows from Wojciech Mula's library at
* https://github.com/WojciechMula/base64simd (published under BSD)
* as well as code from Alfred Klomp's library https://github.com/aklomp/base64 (published under BSD)
*
*/




/**
* Note : Hardware such as Knights Landing might do poorly with this AVX2 code since it relies on shuffles. Alternatives might be faster.
*/


static inline __m256i _mm256_bswap_epi32(const __m256i in) {
  // _mm256_shuffle_epi8() works on two 128-bit lanes separately:
  return _mm256_shuffle_epi8(in, _mm256_setr_epi8(3, 2, 1, 0, 7, 6, 5, 4, 11,
                                                  10, 9, 8, 15, 14, 13, 12, 3,
                                                  2, 1, 0, 7, 6, 5, 4, 11, 10,
                                                  9, 8, 15, 14, 13, 12));
}

static inline __m256i enc_reshuffle(__m256i in) {

  in = _mm256_permutevar8x32_epi32(in,
                                   _mm256_setr_epi32(0, 1, 2, -1, 3, 4, 5, -1));
  in = _mm256_shuffle_epi8(in,
                           _mm256_set_epi8(-1, 9, 10, 11, -1, 6, 7, 8, -1, 3, 4,
                                           5, -1, 0, 1, 2, -1, 9, 10, 11, -1, 6,
                                           7, 8, -1, 3, 4, 5, -1, 0, 1, 2));

  const __m256i merged = _mm256_blend_epi16(_mm256_slli_epi32(in, 4), in, 0x55);

  const __m256i bd = _mm256_and_si256(merged, _mm256_set1_epi32(0x003F003F));
  const __m256i ac = _mm256_and_si256(_mm256_slli_epi32(merged, 2),
                                      _mm256_set1_epi32(0x3F003F00));
  const __m256i indices = _mm256_or_si256(ac, bd);
  return _mm256_bswap_epi32(indices);
}

static inline __m256i enc_translate(const __m256i in) {
  const __m256i lut = _mm256_setr_epi8(
      65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0, 65, 71,
      -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);
  __m256i indices = _mm256_subs_epu8(in, _mm256_set1_epi8(51));
  __m256i mask = _mm256_cmpgt_epi8((in), _mm256_set1_epi8(25));
  indices = _mm256_sub_epi8(indices, mask);
  __m256i out = _mm256_add_epi8(in, _mm256_shuffle_epi8(lut, indices));
  return out;
}

static inline __m256i dec_reshuffle(__m256i in) {

  // inlined procedure pack_madd from https://github.com/WojciechMula/base64simd/blob/master/decode/pack.avx2.cpp
  // The only difference is that elements are reversed,
  // only the multiplication constants were changed.

  const __m256i merge_ab_and_bc = _mm256_maddubs_epi16(in, _mm256_set1_epi32(0x01400140));
  __m256i out = _mm256_madd_epi16(merge_ab_and_bc, _mm256_set1_epi32(0x00011000));
  // end of inlined

  // Pack bytes together within 32-bit words, discarding words 3 and 7:
  out = _mm256_shuffle_epi8(out, _mm256_setr_epi8(
        2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1,
        2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1
  ));

  return _mm256_permutevar8x32_epi32(
      out, _mm256_setr_epi32(0, 1, 2, 4, 5, 6, -1, -1));
}


size_t expavx2_base64_encode(char* dest, const char* str, size_t len) {
      size_t outlen  = 0;
      while (len >= 32) {
        // Load string:
        __m256i inputvector = _mm256_loadu_si256((__m256i *)str);
        // Reshuffle:
        inputvector = enc_reshuffle(inputvector);
        // Translate reshuffled bytes to the Base64 alphabet:
        inputvector = enc_translate(inputvector);
        _mm256_storeu_si256((__m256i *)dest, inputvector);
        str += 24;
        dest += 32;
        outlen += 32;
        len -= 24;
      }
      size_t scalarret = chromium_base64_encode(dest, str, len);
      if(scalarret == MODP_B64_ERROR) return MODP_B64_ERROR;
      return outlen + scalarret;
}

size_t expavx2_base64_decode(char *out, const char *src, size_t srclen) {
      size_t outlen  = 0;
      while (srclen >= 45) {

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

        __m256i str = _mm256_loadu_si256((__m256i *)src);

        // inlined function lookup_pshufb from https://github.com/WojciechMula/base64simd/blob/master/decode/lookup.avx2.cpp
        // description http://0x80.pl/notesen/2016-01-17-sse-base64-decoding.html#vector-lookup-pshufb-update

        const __m256i higher_nibble = _mm256_and_si256(_mm256_srli_epi32(str, 4), _mm256_set1_epi8(0x0f));
        const char linv = 1;
        const char hinv = 0;

        const __m256i lower_bound_LUT = _mm256_setr_epi8(
            /* 0 */ linv, /* 1 */ linv, /* 2 */ 0x2b, /* 3 */ 0x30,
            /* 4 */ 0x41, /* 5 */ 0x50, /* 6 */ 0x61, /* 7 */ 0x70,
            /* 8 */ linv, /* 9 */ linv, /* a */ linv, /* b */ linv,
            /* c */ linv, /* d */ linv, /* e */ linv, /* f */ linv,

            /* 0 */ linv, /* 1 */ linv, /* 2 */ 0x2b, /* 3 */ 0x30,
            /* 4 */ 0x41, /* 5 */ 0x50, /* 6 */ 0x61, /* 7 */ 0x70,
            /* 8 */ linv, /* 9 */ linv, /* a */ linv, /* b */ linv,
            /* c */ linv, /* d */ linv, /* e */ linv, /* f */ linv
        );

        const __m256i upper_bound_LUT = _mm256_setr_epi8(
            /* 0 */ hinv, /* 1 */ hinv, /* 2 */ 0x2b, /* 3 */ 0x39,
            /* 4 */ 0x4f, /* 5 */ 0x5a, /* 6 */ 0x6f, /* 7 */ 0x7a,
            /* 8 */ hinv, /* 9 */ hinv, /* a */ hinv, /* b */ hinv,
            /* c */ hinv, /* d */ hinv, /* e */ hinv, /* f */ hinv,

            /* 0 */ hinv, /* 1 */ hinv, /* 2 */ 0x2b, /* 3 */ 0x39,
            /* 4 */ 0x4f, /* 5 */ 0x5a, /* 6 */ 0x6f, /* 7 */ 0x7a,
            /* 8 */ hinv, /* 9 */ hinv, /* a */ hinv, /* b */ hinv,
            /* c */ hinv, /* d */ hinv, /* e */ hinv, /* f */ hinv
        );

        const __m256i shift_LUT = _mm256_setr_epi8(
            /* 0 */ 0x00,        /* 1 */ 0x00,        /* 2 */ 0x3e - 0x2b, /* 3 */ 0x34 - 0x30,
            /* 4 */ 0x00 - 0x41, /* 5 */ 0x0f - 0x50, /* 6 */ 0x1a - 0x61, /* 7 */ 0x29 - 0x70,
            /* 8 */ 0x00,        /* 9 */ 0x00,        /* a */ 0x00,        /* b */ 0x00,
            /* c */ 0x00,        /* d */ 0x00,        /* e */ 0x00,        /* f */ 0x00,

            /* 0 */ 0x00,        /* 1 */ 0x00,        /* 2 */ 0x3e - 0x2b, /* 3 */ 0x34 - 0x30,
            /* 4 */ 0x00 - 0x41, /* 5 */ 0x0f - 0x50, /* 6 */ 0x1a - 0x61, /* 7 */ 0x29 - 0x70,
            /* 8 */ 0x00,        /* 9 */ 0x00,        /* a */ 0x00,        /* b */ 0x00,
            /* c */ 0x00,        /* d */ 0x00,        /* e */ 0x00,        /* f */ 0x00
        );

        const __m256i upper_bound = _mm256_shuffle_epi8(upper_bound_LUT, higher_nibble);
        const __m256i lower_bound = _mm256_shuffle_epi8(lower_bound_LUT, higher_nibble);

        const __m256i below = _mm256_cmpgt_epi8(lower_bound, str);
        const __m256i above = _mm256_cmpgt_epi8(str, upper_bound);
        const __m256i eq_2f = _mm256_cmpeq_epi8(str, _mm256_set1_epi8(0x2f));

        const __m256i outside = _mm256_andnot_si256(eq_2f, _mm256_or_si256(above, below));

        if (_mm256_movemask_epi8(outside)) {
            break;
        }
        srclen -= 32;
        src += 32;

        const __m256i shift  = _mm256_shuffle_epi8(shift_LUT, higher_nibble);
        const __m256i t0     = _mm256_add_epi8(str, shift);
        str = _mm256_add_epi8(t0, _mm256_and_si256(eq_2f, _mm256_set1_epi8(-3)));

        // end of inlined function

        // Reshuffle the input to packed 12-byte output format:
        str = dec_reshuffle(str);
        _mm256_storeu_si256((__m256i *)out, str);
        out += 24;
        outlen += 24;
      }
      size_t scalarret = chromium_base64_decode(out, src, srclen);
      if(scalarret == MODP_B64_ERROR) return MODP_B64_ERROR;
      return outlen + scalarret;
}
