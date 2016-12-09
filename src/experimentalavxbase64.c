#include "avxbase64.h"
#include <x86intrin.h>

// end-of-file definitions
// Almost end-of-file when waiting for the last '=' character:
#define BASE64_AEOF 1
// End-of-file when stream end has been reached or invalid input provided:
#define BASE64_EOF 2
/**
* Simplified code based on Kompf's
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
    255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255,
    255, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 254,
    255, 255,
    255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255,
    255, 255,
    255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
    39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255,
    255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255,
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
 unsigned char carry;
};

struct codec {
  void (*enc)(struct base64_state *state, const char *src, size_t srclen,
              char *out, size_t *outlen);
  int (*dec)(struct base64_state *state, const char *src, size_t srclen,
             char *out, size_t *outlen);
};

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
  // it should be 4/3 the length of src.
  const uint8_t *c = (const uint8_t *)src;
  uint8_t *o = (uint8_t *)out;
  size_t outl = 0;
  struct base64_state st;
  st.bytes = state->bytes;
  st.carry = state->carry;

  // Turn three bytes into four 6-bit numbers:
  // in[0] = 00111111
  // in[1] = 00112222
  // in[2] = 00222233
  // in[3] = 00333333

  // Duff's device, a for() loop inside a switch() statement.
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
        _mm256_storeu_si256((__m256i *)o, str);
        c += 24;
        o += 32;
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
  size_t outl = 0;
  // Use local temporaries to avoid cache thrashing:
  struct base64_state st;
  st.eof = state->eof;
  st.bytes = state->bytes;
  st.carry = state->carry;
  // If we previously saw an EOF or an invalid character, bail out:
  if (st.eof) {
    *outlen = 0;
    ret = 0;
    if (srclen && (st.eof == 1)) {
      state->bytes = 0;
      state->eof = 2;
      ret = ((base64_table_dec[*c++] == 254) && (srclen == 1)) ? 1 : 0;
    }
    return ret;
  }
    // Turn four 6-bit numbers into three bytes:
    // out[0] = 11111122
    // out[1] = 22223333
    // out[2] = 33444444

    // Duff's device:
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
        srclen -= 32;

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

        __m256i str = _mm256_loadu_si256((__m256i *)c);
        c += 32;

        const __m256i set1 = _mm256_cmpeq_epi8((str), _mm256_set1_epi8('+'));
        const __m256i set2 = _mm256_cmpeq_epi8((str), _mm256_set1_epi8('/'));
        const __m256i set3 = _mm256_andnot_si256(_mm256_cmpgt_epi8(((str)), _mm256_set1_epi8(('9'))), _mm256_cmpgt_epi8(((str)), _mm256_set1_epi8(('0')-1)));
        const __m256i set4 = _mm256_andnot_si256(_mm256_cmpgt_epi8(((str)), _mm256_set1_epi8(('Z'))), _mm256_cmpgt_epi8(((str)), _mm256_set1_epi8(('A')-1)));
        const __m256i set5 = _mm256_andnot_si256(_mm256_cmpgt_epi8(((str)), _mm256_set1_epi8(('z'))), _mm256_cmpgt_epi8(((str)), _mm256_set1_epi8(('a')-1)));
        __m256i delta = _mm256_and_si256((set1), _mm256_set1_epi8(19));
        delta = _mm256_or_si256(delta, _mm256_and_si256((set2), _mm256_set1_epi8(16)));
        delta = _mm256_or_si256(delta, _mm256_and_si256((set3), _mm256_set1_epi8(4)));
        delta = _mm256_or_si256(delta, _mm256_and_si256((set4), _mm256_set1_epi8(-65)));
        delta = _mm256_or_si256(delta, _mm256_and_si256((set5), _mm256_set1_epi8(-71)));
        // Check for invalid input: if any of the delta values are zero,
        // fall back on bytewise code to do error checking and reporting:
        if (_mm256_movemask_epi8(_mm256_cmpeq_epi8((delta), _mm256_setzero_si256()))) {
          break;
        }
        // Now simply add the delta values to the input:
        str = _mm256_add_epi8(str, delta);
        // Reshuffle the input to packed 12-byte output format:
        str = dec_reshuffle(str);
        _mm256_storeu_si256((__m256i *)o, str);
        o += 24;
        outl += 24;
      }
      if (srclen-- == 0) {
        ret = 1;
        break;
      }
      if ((q = base64_table_dec[*c++]) >= 254) {
        st.eof = 2;
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
        st.eof = 2;
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
        if (q == 254) {
          if (srclen-- != 0) {
            st.bytes = 0;
            st.eof = 2;
            q = base64_table_dec[*c++];
            ret = ((q == 254) && (srclen == 0)) ? 1 : 0;
            break;
          } else {
            st.eof = 1;
            ret = 1;
            break;
          }
        }
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
        st.eof = 2;
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

static void base64_stream_encode_init(struct base64_state *state) {
  state->eof = 0;
  state->bytes = 0;
  state->carry = 0;
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
}

static int base64_stream_decode(struct base64_state *state, const char *src,
                         size_t srclen, char *out, size_t *outlen) {
  return base64_stream_decode_avx2(state, src, srclen, out, outlen);
}

void expavx2_base64_encode(const char *src, size_t srclen, char *out, size_t *outlen) {
  size_t s;
  size_t t;
  struct base64_state state;
  base64_stream_encode_init(&state);
  base64_stream_encode(&state, src, srclen, out, &s);
  base64_stream_encode_final(&state, out + s, &t);
  *outlen = s + t;
}

int expavx2_base64_decode(const char *src, size_t srclen, char *out, size_t *outlen) {
  int ret;
  struct base64_state state;
  base64_stream_decode_init(&state);
  ret = base64_stream_decode(&state, src, srclen, out, outlen);
  if (ret && (state.bytes == 0)) {
    return ret;
  }
  return 0;
}
