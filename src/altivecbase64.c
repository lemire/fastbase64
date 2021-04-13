#include "altivecbase64.h"

#include <altivec.h>
#include <stdbool.h>

typedef __attribute__((__aligned__(16), __may_alias__)) vector unsigned char uint8x16;
typedef __vector unsigned short uint16x8;
typedef __vector unsigned int uint32x4;

static inline uint8x16 set1_uint8x16(unsigned char a) {
  return (uint8x16){a, a, a, a, a, a, a, a, a, a, a, a, a, a, a, a};
}

static inline uint8x16 set1_uint16x8(unsigned short a) { 
  return (uint8x16)(uint16x8){a, a, a, a, a, a, a, a};
}

static inline uint8x16 set1_uint32x4(int a) { 
  return (uint8x16)(uint32x4){a, a, a, a};
}

static inline uint8x16 madd_uint8x16(uint8x16 a, uint8x16 b) {
  const uint32x4 zero = {0, 0, 0, 0};
  return (uint8x16)vec_vmsumuhm((uint16x8)a, (uint16x8)b, zero);
}

static inline uint8x16 enc_reshuffle(const uint8x16 input) {
  const uint8x16 perm_mask = {1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10};
  const uint8x16 in = vec_perm(input, input, perm_mask);

  const uint8x16 t0 = vec_and(in, (uint8x16)set1_uint32x4(0x0fc0fc00));
  const uint8x16 t1 = vec_sr((uint16x8)t0, (uint16x8)set1_uint32x4(0x6000a));

  const uint8x16 t2 = vec_and(in, (uint8x16)set1_uint32x4(0x003f03f0));
  const uint8x16 t3 = vec_sl((uint16x8)t2, (uint16x8)set1_uint32x4(0x80004));

  return vec_or(t1, t3);
}

static inline uint8x16 enc_translate(const uint8x16 in) {
  const uint8x16 lut = {65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0};
  uint8x16 indices = vec_subs(in, set1_uint8x16(51));
  const uint8x16 mask = vec_cmpgt((__vector char)in, (__vector char)set1_uint8x16(25));
  indices = vec_sub(indices, mask);
  const uint8x16 out = vec_add(in, vec_perm(lut, lut, indices));
  return out;
}

static inline uint8x16 dec_reshuffle(uint8x16 in) {
  const uint8x16 mask = set1_uint16x8(0x3f);
  // for each 4 bytes [00dddddd|00cccccc|00bbbbbb|00aaaaaa] we do:
  // [00000000|00cccccc|00000000|00aaaaaa]
  const uint8x16 c_and_a = vec_and(in, mask);
  // [0000cccc|cc000000|0000aaaa|aa000000]
  const uint8x16 c_and_a_shifted = vec_sl((uint16x8)c_and_a, (uint16x8)set1_uint16x8(6));
  // [00000000|00dddddd|00000000|00bbbbbb]
  const uint8x16 d_and_b_shifted = vec_sr((uint16x8)in, (uint16x8)set1_uint16x8(8));
  // [0000cccc|ccdddddd|0000aaaa|aabbbbbb]
  const uint8x16 cd_and_ab = vec_or(c_and_a_shifted, d_and_b_shifted);
  // [00000000|aaaaaabb|bbbbcccc|ccdddddd]
  const uint8x16 abcd = madd_uint8x16(cd_and_ab, set1_uint32x4(0x00011000));
  // [4 zero bytes|next 3 bytes|next 3 bytes|next 3 bytes|ddddddcc|ccccbbbb|bbaaaaaa]
  return vec_perm(abcd, abcd, (uint8x16){2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1});
}

size_t altivec_base64_encode(char *dest, const char *str, size_t len) {
  const char *const dest_orig = dest;
  uint8x16 inputvector;
  // pick off 12 bytes at a time for as long as we can.
  // But because we read 16 bytes at a time, ensure we have enough room to
  // do a full 16-byte read without segfaulting:
  while (len >= 16) {
    inputvector = vec_vsx_ld(0, (signed int const *)str);
    inputvector = enc_reshuffle(inputvector);
    inputvector = enc_translate(inputvector);
    vec_vsx_st(inputvector, 0, (uint8x16 *)dest);
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

    uint8x16 str = vec_vsx_ld(0, (signed int const *)src);

    // code by @aqrit from
    // https://github.com/WojciechMula/base64simd/issues/3#issuecomment-271137490
    // transated into AltiVec
    const uint8x16 lut_lo = {0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                         0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A};
    const uint8x16 lut_hi = {0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
                         0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
    const uint8x16 lut_roll = {0, 16, 19, 4, -65, -65, -71, -71, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint8x16 mask_2F = set1_uint8x16(0x2f);
    // lookup
    uint8x16 hi_nibbles = vec_sr((uint32x4)str, vec_splat_u32(4));
    uint8x16 lo_nibbles = vec_and(str, mask_2F);
    const uint8x16 lo = vec_perm(lut_lo, lut_lo, lo_nibbles);
    const uint8x16 eq_2F = vec_cmpeq(str, mask_2F);
    hi_nibbles = vec_and(hi_nibbles, mask_2F);
    const uint8x16 hi = vec_perm(lut_hi, lut_hi, hi_nibbles);
    const uint8x16 roll = vec_perm(lut_roll, lut_roll, vec_add(eq_2F, hi_nibbles));

    if (vec_any_ne(vec_and(lo, hi), set1_uint8x16(0))) {
      break;
    }

    str = vec_add(str, roll);
    // end of copied function

    srclen -= 16;
    src += 16;
    // end of inlined function

    // Reshuffle the input to packed 12-byte output format:
    str = dec_reshuffle(str);
    vec_vsx_st(str, 0, (uint8x16 *)out);
    out += 12;
  }
  size_t scalarret = chromium_base64_decode(out, src, srclen);
  if (scalarret == MODP_B64_ERROR)
    return MODP_B64_ERROR;
  return (out - out_orig) + scalarret;
}
