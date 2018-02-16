#include <arm_neon.h>
#include <stdlib.h>
#include <string.h>
#define MODP_B64_ERROR ((size_t)-1)

size_t chromium_base64_decode(char* dest, const char* src, size_t len);

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

int neon_base64_decode(char *out, const char *src, size_t srclen) {
  char *out_orig = out;
  const uint8x8_t zero8 = vdup_n_u8(0);
  const uint16x8_t zero16 = vdupq_n_u16(0);
  const uint32x4_t zero32 = vdupq_n_u32(0);
  const uint8x8x2_t lut_lo = {0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                              0x11, 0x11, 0x13, 0x1A, 0x1B, 0x1B, 0x1B, 0x1A};
  const uint8x8x2_t lut_hi = {0x10, 0x10, 0x01, 0x02, 0x04, 0x08, 0x04, 0x08,
                              0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
  const uint8x8x2_t lut_roll = {0, 16, 19, 4, 191, 191, 185, 185,
                                0, 0,  0,  0, 0,   0,   0,   0};
  const uint8x8_t cst = {0x40, 0x1, 0x40, 0x1, 0x40, 0x1, 0x40, 0x1};
  const uint16x4_t cst1 = {0x1000, 0x1, 0x1000, 0x1};
  const uint8x8_t k2f = vdup_n_u8(0x2f);
  const uint8x8_t kf = vdup_n_u8(0xf);

  const uint8x8_t shuf = {2, 1, 0, 6, 5, 4, 255, 255};
  const uint8x8_t shuf0 = {2, 1, 0, 6, 5, 4, 2 + 8, 1 + 8};
  const uint8x8_t shuf1 = {0 + 8,  6 + 8,  5 + 8,  4 + 8,
                           2 + 16, 1 + 16, 0 + 16, 6 + 16};
  const uint8x8_t shuf2 = {5 + 16, 4 + 16, 2 + 24, 1 + 24,
                           0 + 24, 6 + 24, 5 + 24, 4 + 24};

  uint8x8x4_t pack;
  uint8x8_t res[3];
  uint8x8_t str[4];

  while (srclen >= 8*4) {
    __builtin_memcpy(str, src, 8*4);
    for (int i = 0; i < 4; i++) {
      uint8x8_t in = str[i];
      uint8x8_t lo_nibbles = vand_u8(in, kf);
      uint8x8_t lo = vtbl2_u8(lut_lo, lo_nibbles);

      uint8x8_t hi_nibbles = vsra_n_u8(zero8, in, 4);
      uint8x8_t hi = vtbl2_u8(lut_hi, hi_nibbles);

      if ((uint64_t)vtst_u8(lo, hi))
        goto break_end;

      uint8x8_t eq_2F = vceq_u8(in, k2f);
      uint8x8_t roll = vtbl2_u8(lut_roll, vadd_u8(eq_2F, hi_nibbles));
      uint8x8_t rolled = vadd_u8(in, roll);

      // Step 1: swap and merge adjacent 6-bit fields.
      uint16x8_t mul = vmlal_u8(zero16, rolled, cst);
      uint32x4_t t = vpaddlq_u16(mul);
      uint8x8_t merge = vuzp_u16(vget_low_u32(t), vget_high_u32(t)).val[0];

      // Step 2: swap and merge 12-bit words into a 24-bit word.
      uint32x4_t v = vpaddlq_u32(vmlal_u16(zero32, merge, cst1));
      uint8x8_t merge1 = vuzp_u32(vget_low_u32(v), vget_high_u32(v)).val[0];

      pack.val[i] = merge1;
    }

    res[0] = vtbl4_u8(pack, shuf0);
    res[1] = vtbl4_u8(pack, shuf1);
    res[2] = vtbl4_u8(pack, shuf2);
    __builtin_memcpy(out, res, 6*4);

    out += 6*4;
    srclen -= 8*4;
    src += 8*4;
  }

  break_end:
  size_t scalarret = chromium_base64_decode(out, src, srclen);
  if (scalarret == MODP_B64_ERROR)
    return (int) MODP_B64_ERROR;
  return (out - out_orig) + scalarret;
}
