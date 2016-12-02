#include "avxbase64.h"

#include <assert.h>
#include <x86intrin.h>


static inline __m256i  packed_dword(uint32_t x) {
  return _mm256_set1_epi32(x);
}
static inline __m256i  packed_byte(char b) {
  return _mm256_set1_epi8(b);
}


static inline __m256i pack_madd_improved(const __m256i values) {

    //          packed_dword([0000dddd|ddcccccc|0000bbbb|bbaaaaaa] x 4)
    const __m256i merge_ab_and_bc = _mm256_maddubs_epi16(values, packed_dword(0x40014001));

    // bits_cd: packed_dword([0000dddd|ddcccccc|00000000|00000000] x 4)
    // bits_ab: packed_dword([00000000|00000000|0000bbbb|bbaaaaaa] x 4)
    const __m256i bits_cd = _mm256_and_si256(merge_ab_and_bc, packed_dword(0x0fff0000));
    const __m256i bits_ab = _mm256_and_si256(merge_ab_and_bc, packed_dword(0x00000fff));

    // merged:  packed_dword([00000000|ddddddcc|ccccbbbb|bbaaaaaa] x 4)
    return _mm256_or_si256(bits_ab, _mm256_srli_epi32(bits_cd, 4));
}


static inline __m256i lookup_pshufb(const __m256i input) {

    const __m256i row_LUT = _mm256_setr_epi8(
        /*  0 ..  9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 10 .. 12 */ 4*1, 4*1, 4*1,
        /* 13 .. 14 */ 4*2, 4*2,
        /*       15 */ 4*3,
        /*  0 ..  9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 10 .. 12 */ 4*1, 4*1, 4*1,
        /* 13 .. 14 */ 4*2, 4*2,
        /*       15 */ 4*3
    );

    const __m256i bits_1_4    = input & packed_byte(0x1e);
    const __m256i bit_0_and_5 = input ^ bits_1_4;
    const __m256i index1 = _mm256_srli_epi16(bits_1_4, 1);
    const __m256i row_number = _mm256_shuffle_epi8(row_LUT, index1);
    const __m256i index2 = bit_0_and_5 | (_mm256_srli_epi16(bit_0_and_5, 4));
    const __m256i shift_index = row_number | index2;

    const __m256i shift_LUT = _mm256_setr_epi8(
        /* 0 */ 65, 65,  71,  71,
        /* 1 */ 65, 65,  -4,  -4,
        /* 2 */ 71, 71,  -4,  -4,
        /* 3 */ 71, 71, -19, -16,
        /* 0 */ 65, 65,  71,  71,
        /* 1 */ 65, 65,  -4,  -4,
        /* 2 */ 71, 71,  -4,  -4,
        /* 3 */ 71, 71, -19, -16
    );

    const __m256i shift = _mm256_shuffle_epi8(shift_LUT, shift_index);

    return _mm256_add_epi8(input, shift);
}


/**
* "size" is the size of the input
* should, theoretically, return the number of written bytes
*/
size_t avx_b64_decode(char* out, const char* input, size_t size) {

    assert(size % 32 == 0);
    char * initout = out;


    for (size_t i=0; i < size; i += 32) {

        __m256i in = _mm256_loadu_si256((const __m256i*)(input + i));
        __m256i values;

        values = lookup_pshufb(in);

        // input:  packed_dword([00dddddd|00cccccc|00bbbbbb|00aaaaaa] x 4)
        // merged: packed_dword([00000000|ddddddcc|ccccbbbb|bbaaaaaa] x 4)

        const __m256i merged = pack_madd_improved(values);

        // merged = packed_byte([0XXX|0YYY|0ZZZ|0WWW])

        const __m256i shuf = _mm256_setr_epi8(
               0,  1,  2,
               4,  5,  6,
               8,  9, 10,
              12, 13, 14,
              0xff, 0xff, 0xff, 0xff,
               0,  1,  2,
               4,  5,  6,
               8,  9, 10,
              12, 13, 14,
              0xff, 0xff, 0xff, 0xff
        );

        const __m256i shuffled = _mm256_shuffle_epi8(merged, shuf);

        _mm_storeu_si128((__m128i*)(out), _mm256_extracti128_si256(shuffled, 0));
        _mm_storeu_si128((__m128i*)(out + 12), _mm256_extracti128_si256(shuffled, 1));

        out += 24;
    }
    return out - initout;
}
