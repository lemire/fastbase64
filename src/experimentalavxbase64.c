#include "avxbase64.h"
#include <x86intrin.h>

static const uint8_t base64_table_enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";
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
  const __m256i maskB2 = _mm256_set1_epi32(0x003F0000);
  const __m256i maskB1 = _mm256_set1_epi32(0x00003F00);
  __m256i out = _mm256_srli_epi32(in, 16);
  out =
      _mm256_or_si256(out, _mm256_srli_epi32(_mm256_and_si256(in, maskB2), 2));
  out =
      _mm256_or_si256(out, _mm256_slli_epi32(_mm256_and_si256(in, maskB1), 12));
  out = _mm256_or_si256(out, _mm256_slli_epi32(in, 26));
  out = _mm256_shuffle_epi8(out, _mm256_setr_epi8(3, 2, 1, 7, 6, 5, 11, 10, 9,
                                                  15, 14, 13, -1, -1, -1, -1, 3,
                                                  2, 1, 7, 6, 5, 11, 10, 9, 15,
                                                  14, 13, -1, -1, -1, -1));
  return _mm256_permutevar8x32_epi32(
      out, _mm256_setr_epi32(0, 1, 2, 4, 5, 6, -1, -1));
}

static void base64_stream_encode_avx2(struct base64_state *state, const char *src,
                               size_t srclen, char *out, size_t *outlen) {
  const uint8_t *c = (const uint8_t *)src;
  uint8_t *o = (uint8_t *)out;
  size_t outl = 0;
  struct base64_state st;
  st.bytes = state->bytes;
  st.carry = state->carry;
  switch (st.bytes) {
    for (;;) {
    case 0:
      while (srclen >= 32) {
        __m256i str = _mm256_loadu_si256((__m256i *)c);
        str = enc_reshuffle(str);
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
  struct base64_state st;
  st.eof = state->eof;
  st.bytes = state->bytes;
  st.carry = state->carry;
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
  switch (st.bytes) {
    for (;;) {
    case 0:
      while (srclen >= 45) {
        __m256i str = _mm256_loadu_si256((__m256i *)c);
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
        if (_mm256_movemask_epi8(_mm256_cmpeq_epi8((delta), _mm256_set1_epi8(0)))) {
          break;
        }
        str = _mm256_add_epi8(str, delta);
        str = dec_reshuffle(str);
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
