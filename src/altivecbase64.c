#include "altivecbase64.h"

#include <altivec.h>
#include <stdbool.h>

typedef __attribute__((__aligned__(16), __may_alias__)) vector unsigned char v16i;
typedef __vector short v8hi;
typedef __vector unsigned short v8hu;
typedef __vector int v4si;
typedef __vector unsigned int v4su;

static inline v16i _mm_set1_epi8(char a) {
  return (v16i){a, a, a, a, a, a, a, a, a, a, a, a, a, a, a, a};
}

static inline v16i _mm_set1_epi16(short a) {
  return (v16i)(v8hi){a, a, a, a, a, a, a, a};
}

static inline v16i _mm_set1_epi32(int a) {
  return (v16i)(v4si){a, a, a, a};
}

static inline v16i _mm_madd_epi16(v16i a, v16i b) {
  const v4si zero = {0, 0, 0, 0};
  return (v16i)vec_vmsumshm((v8hi)a, (v8hi)b, zero);
}

static inline v16i enc_reshuffle(const v16i input) {
  const v16i perm_mask = {
      1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10,
  };
  const v16i in = vec_perm(input, input, perm_mask);

  const v16i t0 = vec_and(in, (v16i)_mm_set1_epi32(0x0fc0fc00));
  const v16i t1 = vec_sr((v8hu)t0, (v8hu)_mm_set1_epi32(0x6000a));

  const v16i t2 = vec_and(in, (v16i)_mm_set1_epi32(0x003f03f0));
  const v16i t3 = vec_sl((v8hu)t2, (v8hu)_mm_set1_epi32(0x80004));

  return vec_or(t1, t3);
}

static inline v16i enc_translate(const v16i in) {
  const v16i lut = {65, 71, -4, -4, -4,  -4,  -4, -4,
                       -4, -4, -4, -4, -19, -16, 0,  0};
  v16i indices = vec_subs(in, _mm_set1_epi8(51));
  const v16i mask =
      vec_cmpgt((__vector char)in, (__vector char)_mm_set1_epi8(25));
  indices = vec_sub(indices, mask);
  const v16i out = vec_add(in, vec_perm(lut, lut, indices));
  return out;
}

static inline v16i dec_reshuffle(v16i in) {
  const v16i mask = _mm_set1_epi16(0x3f);

  const v16i only_c_and_a = vec_and(in, mask);
  const v16i only_c_and_a_shifted =
      vec_sl((v8hu)only_c_and_a, (v8hu)_mm_set1_epi16(6));

  const v16i d_and_b_shifted = vec_sr((v8hu)in, (v8hu)_mm_set1_epi16(8));
  const v16i only_d_and_b_shifted = vec_and(d_and_b_shifted, mask);

  const v16i cd_and_ab = vec_or(only_c_and_a_shifted, only_d_and_b_shifted);

  const v16i abcd = _mm_madd_epi16(cd_and_ab, _mm_set1_epi32(0x00011000));
  return vec_perm(
      abcd, abcd,
      (v16i){2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1});
}

size_t altivec_base64_encode(char *dest, const char *str, size_t len) {
  const char *const dest_orig = dest;
  v16i inputvector;
  // pick off 12 bytes at a time for as long as we can.
  // But because we read 16 bytes at a time, ensure we have enough room to
  // do a full 16-byte read without segfaulting:
  while (len >= 16) {
    inputvector = vec_vsx_ld(0, (signed int const *)str);
    inputvector = enc_reshuffle(inputvector);
    inputvector = enc_translate(inputvector);
    vec_vsx_st(inputvector, 0, (v16i *)dest);
    dest += 16;
    str += 12;
    len -= 12;
  }
  size_t scalarret = chromium_base64_encode(dest, str, len);
  if (scalarret == MODP_B64_ERROR)
    return MODP_B64_ERROR;
  return (dest - dest_orig) + scalarret;
}

size_t altivec_base64_decode(char *out, const char *src, size_t srclen) {
  char *out_orig = out;
  // pick off 16 bytes at a time for as long as we can,
  // but make sure that we quit before seeing any == markers at the end of
  // the string. Also, because we write 4 zero bytes at the end of the output
  // ensure that there are at least 6 valid bytes of input data remaining to
  // close the gap. 16 + 2 + 6 = 24 bytes:
  while (srclen >= 24) {
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

    v16i str = vec_vsx_ld(0, (signed int const *)src);

    // code by @aqrit from
    // https://github.com/WojciechMula/base64simd/issues/3#issuecomment-271137490
    // transated into AltiVec
    const v16i lut_lo = {0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                            0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A};
    const v16i lut_hi = {0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
                            0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
    const v16i lut_roll = {0, 16, 19, 4, -65, -65, -71, -71,
                              0, 0,  0,  0, 0,   0,   0,   0};
    const v16i mask_2F = _mm_set1_epi8(0x2f);
    // lookup
    v16i hi_nibbles = vec_sr((v4su)str, vec_splat_u32(4));
    v16i lo_nibbles = vec_and(str, mask_2F);
    const v16i lo = vec_perm(lut_lo, lut_lo, lo_nibbles);
    const v16i eq_2F = vec_cmpeq(str, mask_2F);
    hi_nibbles = vec_and(hi_nibbles, mask_2F);
    const v16i hi = vec_perm(lut_hi, lut_hi, hi_nibbles);
    const v16i roll =
        vec_perm(lut_roll, lut_roll, vec_add(eq_2F, hi_nibbles));

    if (vec_any_ne(vec_and(lo, hi), _mm_set1_epi8(0))) {
      break;
    }

    str = vec_add(str, roll);
    // end of copied function

    srclen -= 16;
    src += 16;
    // end of inlined function

    // Reshuffle the input to packed 12-byte output format:
    str = dec_reshuffle(str);
    vec_vsx_st(str, 0, (v16i *)out);
    out += 12;
  }
  size_t scalarret = chromium_base64_decode(out, src, srclen);
  if (scalarret == MODP_B64_ERROR)
    return MODP_B64_ERROR;
  return (out - out_orig) + scalarret;
}
