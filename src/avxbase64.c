#include "avxbase64.h"

#include <x86intrin.h>


/**
* This function converts base64 text into an array of bytes (in [0,64))
* without support for "end of stream".
*
* from https://github.com/WojciechMula/base64simd
*/
__m256i lookup_pshufb(const __m256i input) {

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

    const __m256i bits_1_4    = input & _mm256_set1_epi8(0x1e);
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
