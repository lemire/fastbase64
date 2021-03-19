#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chromiumbase64.h"
#include "linuxbase64.h"
#include "quicktimebase64.h"
#include "scalarbase64.h"

#ifdef HAVE_AVX2
#include "fastavxbase64.h"
#include "klompavxbase64.h"
#endif // HAVE_AVX2
#ifdef HAVE_AVX512BW
#include "fastavx512bwbase64.h"
#endif // HAVE_AVX512BW
#ifdef ALTIVEC
#include "altivecbase64.h"
#endif // ALTIVEC

void print_example(const char *source) {
  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));
  unsigned int len = chromium_base64_decode(dest1, source, strlen(source));
  unsigned int i = 0;
  for (; i != len; i++)
    printf("%u ", dest1[i] & 0xFF);
  printf("\n");
  free(dest1);
}

void chromium_checkExample(const char *source, const char *coded) {
  printf("chromium codec check.\n");
  unsigned int len;
  unsigned int codedlen;

  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = chromium_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1, coded, codedlen) == 0);
  char *dest2 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = chromium_base64_decode(dest2, coded, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);
  char *dest3 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = chromium_base64_decode(dest3, dest1, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}

void quicktime_checkExample(const char *source, const char *coded) {
  printf("quicktime codec check.\n");
  unsigned int len;
  unsigned int codedlen;

  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = quicktime_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1, coded, codedlen) == 0);
  char *dest2 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = quicktime_base64_decode(dest2, coded);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);
  char *dest3 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = quicktime_base64_decode(dest3, dest1);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}

void linux_checkExample(const char *source, const char *coded) {
  printf("linux codec check.\n");
  unsigned int len;
  unsigned int codedlen;

  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = chromium_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1, coded, codedlen) == 0);
  char *dest2 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = linux_base64_decode(dest2, coded, coded + codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);
  char *dest3 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = linux_base64_decode(dest3, dest1, dest1 + codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}

void scalar_checkExample(const char *source, const char *coded) {
  printf("scalar codec check.\n");
  size_t len;
  size_t codedlen;

  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));

  scalar_base64_encode(source, strlen(source), dest1, &codedlen);
  assert(strncmp(dest1, coded, codedlen) == 0);
  char *dest2 = (char *)malloc(chromium_base64_decode_len(codedlen));
  scalar_base64_decode(coded, codedlen, dest2, &len);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);
  char *dest3 = (char *)malloc(chromium_base64_decode_len(codedlen));
  scalar_base64_decode(dest1, codedlen, dest3, &len);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}

#ifdef HAVE_AVX2
void klomp_avx2_checkExample(const char *source, const char *coded) {
  printf("klomp avx2 codec check.\n");
  size_t len;
  size_t codedlen;

  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));
  klomp_avx2_base64_encode(source, strlen(source), dest1, &codedlen);
  assert(strncmp(dest1, coded, codedlen) == 0);
  char *dest2 = (char *)malloc(chromium_base64_decode_len(codedlen));
  klomp_avx2_base64_decode(coded, codedlen, dest2, &len);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);
  char *dest3 = (char *)malloc(chromium_base64_decode_len(codedlen));
  klomp_avx2_base64_decode(dest1, codedlen, dest3, &len);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}

void fast_avx2_checkExample(const char *source, const char *coded) {
  printf("fast_avx2 codec check.\n");
  size_t len;
  size_t codedlen;

  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = fast_avx2_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1, coded, codedlen) == 0);

  char *dest2 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = fast_avx2_base64_decode(dest2, coded, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);

  char *dest3 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = fast_avx2_base64_decode(dest3, dest1, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);

  free(dest1);
  free(dest2);
  free(dest3);
}
#endif // HAVE_AVX2

#ifdef HAVE_AVX512BW
void fast_avx512bw_checkExample(const char *source, const char *coded) {
  printf("fast_avx512bw codec check.\n");
  size_t len;
  size_t codedlen;

  char *dest1 = (char *)malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = fast_avx512bw_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1, coded, codedlen) == 0);
  char *dest2 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = fast_avx512bw_base64_decode(dest2, coded, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);
  char *dest3 = (char *)malloc(chromium_base64_decode_len(codedlen));
  len = fast_avx512bw_base64_decode(dest3, dest1, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}
#endif // HAVE_AVX512BW

#ifdef ALTIVEC
void altivec_checkExample(const char *source, const char *coded) {
  printf("altivec codec check.\n");
  size_t len;
  size_t codedlen;

  char *dest1 = malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = altivec_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1, coded, codedlen) == 0);

  char *dest2 = malloc(chromium_base64_decode_len(codedlen));
  len = altivec_base64_decode(dest2, coded, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2, source, strlen(source)) == 0);

  char *dest3 = malloc(chromium_base64_decode_len(codedlen));
  len = altivec_base64_decode(dest3, dest1, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3, source, strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}
#endif // ALTIVEC

#ifdef HAVE_AVX2
static const uint8_t base64_table_enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                          "abcdefghijklmnopqrstuvwxyz"
                                          "0123456789+/";
void fast_avx2_checkError() {
  printf("fast_avx2 codec error check.\n");
  char source[64];
  char dest[48];
  for (unsigned int z = 0; z < 64; ++z) {
    for (int i = 0; i < 64; ++i)
      source[i] = base64_table_enc[z];
    int len = fast_avx2_base64_decode(dest, source, 64);
    assert(len == 48);
  }
  for (int z = 0; z < 256; ++z) {
    bool in_list = false;
    for (unsigned int zz = 0; zz < 64; ++zz)
      if (base64_table_enc[zz] == z)
        in_list = true;
    if (!in_list) {
      for (int pos = 0; pos < 32; ++pos) {
        for (int i = 0; i < 64; ++i)
          source[i] = 'A';
        source[pos] = z;
        int len = fast_avx2_base64_decode(dest, source, 64);
        assert(len == -1);
      }
    }
  }
}
#endif // HAVE_AVX2

int main() {
#ifdef HAVE_AVX2
  fast_avx2_checkError();
#endif // HAVE_AVX2

  // from Wikipedia page
  const char *wikipediasource =
      "Man is distinguished, not only by his reason, but by this singular passion from \
other animals, which is a lust of the mind, that by a perseverance of delight \
in the continued and indefatigable generation of knowledge, exceeds the short \
vehemence of any carnal pleasure.";
  const char *wikipediacoded =
      "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz\
IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg\
dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu\
dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo\
ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";

  // from https://gobyexample.com/base64-encoding
  const char *gosource = "abc123!?$*&()'-=@~";
  const char *gocoded = "YWJjMTIzIT8kKiYoKSctPUB+";

  // from https://www.tutorialspoint.com/java8/java8_base64.htm
  const char *tutosource = "TutorialsPoint?java8";
  const char *tutocoded = "VHV0b3JpYWxzUG9pbnQ/amF2YTg=";

  chromium_checkExample(wikipediasource, wikipediacoded);
  chromium_checkExample(gosource, gocoded);
  chromium_checkExample(tutosource, tutocoded);

  scalar_checkExample(wikipediasource, wikipediacoded);
  scalar_checkExample(gosource, gocoded);
  scalar_checkExample(tutosource, tutocoded);

  quicktime_checkExample(wikipediasource, wikipediacoded);
  quicktime_checkExample(gosource, gocoded);
  quicktime_checkExample(tutosource, tutocoded);

  linux_checkExample(wikipediasource, wikipediacoded);
  linux_checkExample(gosource, gocoded);
  linux_checkExample(tutosource, tutocoded);

#ifdef HAVE_AVX2
  klomp_avx2_checkExample(wikipediasource, wikipediacoded);
  klomp_avx2_checkExample(gosource, gocoded);
  klomp_avx2_checkExample(tutosource, tutocoded);

  fast_avx2_checkExample(wikipediasource, wikipediacoded);
  fast_avx2_checkExample(gosource, gocoded);
  fast_avx2_checkExample(tutosource, tutocoded);
#endif // HAVE_AVX2

#ifdef HAVE_AVX512BW
  fast_avx512bw_checkExample(wikipediasource, wikipediacoded);
  fast_avx512bw_checkExample(gosource, gocoded);
  fast_avx512bw_checkExample(tutosource, tutocoded);
#endif // HAVE_AVX512BW

#ifdef ALTIVEC
  altivec_checkExample(wikipediasource, wikipediacoded);
  altivec_checkExample(gosource, gocoded);
  altivec_checkExample(tutosource, tutocoded);
#endif // ALTIVEC

  print_example("R0lGODlhAQABAIAAAP///wAAACwAAAAAAQABAAACAkQBADs=");
  printf("Code looks ok.\n");
  return 0;
}
