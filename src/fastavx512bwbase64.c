#include "fastavx512bwbase64.h"

#include <x86intrin.h>
#include <stdbool.h>

static inline __m512i enc_reshuffle(const __m512i input) {

    // from https://github.com/WojciechMula/base64simd/blob/master/encode/encode.avx512bw.cpp

    // place each 12-byte subarray in seprate 128-bit lane
    // tmp1 = [?? ?? ?? ??|D2 D1 D0 C2|C1 C0 B2 B1|B0 A2 A1 A0] x 4
    //           ignored
    const __m512i tmp1 = _mm512_permutexvar_epi32(
        _mm512_set_epi32(-1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0),
        input
    );

    // reshuffle bytes within 128-bit lanes to format required by
    // AVX512BW unpack procedure
    // tmp2 = [D1 D2 D0 D1|C1 C2 C0 C1|B1 B2 B0 B1|A1 A2 A0 A1] x 4
    //         10 11 9  10 7  8  6  7  4  5  3  4  1  2  0  1
    const __m512i tmp2 = _mm512_shuffle_epi8(
        tmp1,
        _mm512_set4_epi32(0x0a0b090a, 0x07080607, 0x04050304, 0x01020001)
    );

    return tmp2;
}

static inline __m512i enc_translate(const __m512i input) {

    // from https://github.com/WojciechMula/base64simd/blob/master/encode/lookup.avx512bw.cpp

    // reduce  0..51 -> 0
    //        52..61 -> 1 .. 10
    //            62 -> 11
    //            63 -> 12
    __m512i result = _mm512_subs_epu8(input, _mm512_set1_epi8(51));

    // distinguish between ranges 0..25 and 26..51:
    //         0 .. 25 -> remains 0
    //        26 .. 51 -> becomes 13
    const __mmask64 less = _mm512_cmpgt_epi8_mask(_mm512_set1_epi8(26), input);
    result = _mm512_mask_mov_epi8(result, less, _mm512_set1_epi8(13));

    /* the SSE lookup
        const __m128i shift_LUT = _mm_setr_epi8(
            'a' - 26, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
            '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62,
            '/' - 63, 'A', 0, 0
        );
        which is:
            0x47, 0xfc, 0xfc, 0xfc,
            0xfc, 0xfc, 0xfc, 0xfc,
            0xfc, 0xfc, 0xfc, 0xed,
            0xf0, 0x41, 0x00, 0x00

        Note that the order of above list is reserved (due to _mm_setr_epi8),
        so the invocation _mm512_set4_epi32 looks... odd.
    */
    const __m512i shift_LUT = _mm512_set4_epi32(
        0x000041f0,
        0xedfcfcfc,
        0xfcfcfcfc,
        0xfcfcfc47
    );

    // read shift
    result = _mm512_shuffle_epi8(shift_LUT, result);

    return _mm512_add_epi8(result, input);
}

static inline __m512i dec_reshuffle(__m512i input) {

    // from https://github.com/WojciechMula/base64simd/blob/master/decode/pack.avx512bw.cpp
    const __m512i merge_ab_and_bc = _mm512_maddubs_epi16(input, _mm512_set1_epi32(0x01400140));

    return _mm512_madd_epi16(merge_ab_and_bc, _mm512_set1_epi32(0x00011000));
}


size_t fast_avx512bw_base64_encode(char* dest, const char* str, size_t len) {
    const char* const dest_orig = dest;
    __m512i inputvector;
    while (len >= 64) {
        inputvector = _mm512_loadu_si512((__m512i *)(str));
        inputvector = enc_reshuffle(inputvector);
        inputvector = enc_translate(inputvector);
        _mm512_storeu_si512((__m512i *)dest, inputvector);
        str  += 48;
        dest += 64;
        len -= 48;
    }
    size_t scalarret = chromium_base64_encode(dest, str, len);
    if(scalarret == MODP_B64_ERROR) return MODP_B64_ERROR;
    return (dest - dest_orig) + scalarret;
}

#define build_dword(b0, b1, b2, b3)     \
     (((uint32_t)((uint8_t)(b0)) << 0*8)    \
    | ((uint32_t)((uint8_t)(b1)) << 1*8)    \
    | ((uint32_t)((uint8_t)(b2)) << 2*8)    \
    | ((uint32_t)((uint8_t)(b3)) << 3*8))
    
#define _mm512_set4lanes_epi8(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15) \
    _mm512_setr4_epi32(                  \
        build_dword( b0,  b1,  b2,  b3), \
        build_dword( b4,  b5,  b6,  b7), \
        build_dword( b8,  b9, b10, b11), \
        build_dword(b12, b13, b14, b15))

size_t fast_avx512bw_base64_decode(char *out, const char *src, size_t srclen) {
  char* out_orig = out;
  while (srclen >= 88) {

    // load
    const __m512i input = _mm512_loadu_si512((const __m512i*)(src));
        
    // translate from ASCII
    //  and https://github.com/WojciechMula/base64simd/blob/master/decode/lookup.avx512bw.cpp
    const __m512i higher_nibble = _mm512_and_si512(_mm512_srli_epi32(input, 4), _mm512_set1_epi8(0x0f));
    const __m512i lower_nibble  = _mm512_and_si512(input, _mm512_set1_epi8(0x0f));

    const __m512i shiftLUT = _mm512_set4lanes_epi8(
        0,   0,  19,   4, -65, -65, -71, -71,
        0,   0,   0,   0,   0,   0,   0,   0);

    const __m512i maskLUT  = _mm512_set4lanes_epi8(
        /* 0        : 0b1010_1000*/ 0xa8,
        /* 1 .. 9   : 0b1111_1000*/ 0xf8, 0xf8, 0xf8, 0xf8,
                                    0xf8, 0xf8, 0xf8, 0xf8,
                                    0xf8,
        /* 10       : 0b1111_0000*/ 0xf0,
        /* 11       : 0b0101_0100*/ 0x54,
        /* 12 .. 14 : 0b0101_0000*/ 0x50, 0x50, 0x50,
        /* 15       : 0b0101_0100*/ 0x54
    );

    const __m512i bitposLUT = _mm512_set4lanes_epi8(
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    );

    const __m512i   sh      = _mm512_shuffle_epi8(shiftLUT,  higher_nibble);
    const __mmask64 eq_2f   = _mm512_cmpeq_epi8_mask(input, _mm512_set1_epi8(0x2f));
    const __m512i   shift   = _mm512_mask_mov_epi8(sh, eq_2f, _mm512_set1_epi8(16));

    const __m512i M         = _mm512_shuffle_epi8(maskLUT,   lower_nibble);
    const __m512i bit       = _mm512_shuffle_epi8(bitposLUT, higher_nibble);

    const uint64_t match    = _mm512_test_epi8_mask(M, bit);

    if (match != (uint64_t)(-1)) {
        // some characters do not match the valid range
        return MODP_B64_ERROR;
    }

    const __m512i translated = _mm512_add_epi8(input, shift);

    const __m512i packed = dec_reshuffle(translated);

    //  and https://github.com/WojciechMula/base64simd/blob/master/decode/decode.avx512bw.cpp
    // 
    const __m512i t1 = _mm512_shuffle_epi8(
        packed,
        _mm512_set4lanes_epi8(
             2,  1,  0,
             6,  5,  4,
            10,  9,  8,
            14, 13, 12,
            -1, -1, -1, -1)
    );

    // shuffle bytes
    const __m512i s6 = _mm512_setr_epi32(
         0,  1,  2,
         4,  5,  6,
         8,  9, 10,
        12, 13, 14,
        // unused
         0,  0,  0, 0);

    const __m512i t2 = _mm512_permutexvar_epi32(s6, t1);

    _mm512_storeu_si512((__m512i*)(out), t2);

    srclen -= 64;
    src += 64;
    out += 48;
  }
  size_t scalarret = chromium_base64_decode(out, src, srclen);
  if(scalarret == MODP_B64_ERROR) return MODP_B64_ERROR;
  return (out - out_orig) + scalarret;
}

