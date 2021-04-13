#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "benchmark.h"
#include "images_base64.h"
#include "moby_dick.h"
#include "webexamples.h"

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

static const int repeat = 50;

void testencode(const char *data, size_t datalength, bool verbose) {
  if (verbose) {
    printf("encode a base64 input of %zu bytes, ", datalength);
  }

  char *buffer = malloc(datalength * 2); // we allocate plenty of memory
  int expected = (int)chromium_base64_encode(buffer, data, datalength);
  size_t outputlength;
  if (verbose) {
    printf("encoded size = %d \n", expected);
  }
  BEST_TIME_NOCHECK(memcpy(buffer, data, datalength), , repeat, datalength, verbose);
  BEST_TIME_NOCHECK(linux_base64_encode(buffer, data, data + datalength), , repeat, datalength, verbose);
  BEST_TIME_NOCHECK(quicktime_base64_encode(buffer, data, datalength), , repeat, datalength, verbose);
  BEST_TIME(chromium_base64_encode(buffer, data, datalength), expected, , repeat, datalength, verbose);
  BEST_TIME_CHECK(scalar_base64_encode(data, datalength, buffer, &outputlength), ((int)outputlength == expected), ,
                  repeat, datalength, verbose);

#ifdef HAVE_AVX2
  BEST_TIME_CHECK(klomp_avx2_base64_encode(data, datalength, buffer, &outputlength), ((int)outputlength == expected), ,
                  repeat, datalength, verbose);
  BEST_TIME(fast_avx2_base64_encode(buffer, data, datalength), expected, , repeat, datalength, verbose);
#endif // HAVE_AVX2
#ifdef HAVE_AVX512BW
  BEST_TIME(fast_avx512bw_base64_encode(buffer, data, datalength), expected, , repeat, datalength, verbose);
#endif // HAVE_AVX512BW
#ifdef ALTIVEC
  BEST_TIME(altivec_base64_encode(buffer, data, datalength), expected, , repeat, datalength, verbose);
#endif // ALTIVEC

  free(buffer);
  if (verbose) {
    printf("\n");
  }
}

void testdecode(const char *data, size_t datalength, bool verbose) {
  if (verbose) {
    printf("decoding a base64 input of %zu bytes, ", datalength);
  }
  if (datalength < 4 || (datalength % 4 != 0)) {
    printf("size should be divisible by 4 bytes.\n");
    return;
  }

  char *buffer = malloc(datalength * 2); // we allocate plenty of memory
  int expected = (int)chromium_base64_decode(buffer, data, datalength);
  if (verbose) {
    printf("original size = %d \n", expected);
  }
  BEST_TIME_NOCHECK(memcpy(buffer, data, datalength), , repeat, datalength, verbose);
  BEST_TIME(linux_base64_decode(buffer, data, data + datalength), expected, , repeat, datalength, verbose);
  BEST_TIME(quicktime_base64_decode(buffer, data), expected, , repeat, datalength, verbose);
  BEST_TIME(chromium_base64_decode(buffer, data, datalength), expected, , repeat, datalength, verbose);
  size_t outputlength;
  // scalar_base64_decode and klomp_avx2_base64_decode return 0/1, not output length
  int scalarexpected = scalar_base64_decode(data, datalength, buffer, &outputlength);
  assert((int)outputlength == expected);
  BEST_TIME(scalar_base64_decode(data, datalength, buffer, &outputlength), scalarexpected, , repeat, datalength,
            verbose);
#ifdef HAVE_AVX2
  BEST_TIME(klomp_avx2_base64_decode(data, datalength, buffer, &outputlength), scalarexpected, , repeat, datalength,
            verbose);
  BEST_TIME(fast_avx2_base64_decode(buffer, data, datalength), expected, , repeat, datalength, verbose);
#endif // HAVE_AVX2
#ifdef HAVE_AVX512BW
  BEST_TIME(fast_avx512bw_base64_decode(buffer, data, datalength), expected, , repeat, datalength, verbose);
#endif // HAVE_AVX512BW
#ifdef ALTIVEC
  BEST_TIME(altivec_base64_decode(buffer, data, datalength), expected, , repeat, datalength, verbose);
#endif // ALTIVEC

  free(buffer);
  if (verbose) {
    printf("\n");
  }
}

int main() {
  // int main(int argc, char **argv) {
#ifdef HAVE_AVX2
  RDTSC_SET_OVERHEAD(rdtsc_overhead_func(1), repeat);
#endif // HAVE_AVX2

  printf("Testing first with random data.\n");
  const int N = 2048;
  char randombuffer[N];
  for (int k = 0; k < N; ++k) {
    randombuffer[k] = rand();
  }
  const char *decodingfilename = "decodingperf.txt";
  const char *encodingfilename = "encodingperf.txt";
  printf("See files %s %s ... \n", encodingfilename, decodingfilename);

  if (freopen(decodingfilename, "w", stdout) == NULL) {
    printf("error opening %s \n", decodingfilename);
  }

#ifdef HAVE_AVX2
#ifndef HAVE_AVX512BW
  printf("#displaying cycles per input bytes for memcpy and linux, quicktime, chromium, scalar, "
         "avx2 (Klomp), avx2 decoders, first column is number of bytes\n");
#endif // do not HAVE_AVX512BW
#endif // HAVE_AVX2
#ifdef HAVE_AVX512BW
  printf("#displaying cycles per input bytes for memcpy and linux, quicktime, chromium, scalar, "
         "avx2 (Klomp), avx2, avx512 decoders, first column is number of bytes\n");
#endif // HAVE_AVX512BW
#ifdef ALTIVEC
  printf("#displaying nanoseconds per input bytes for memcpy and linux, quicktime, chromium, scalar, "
         "altivec decoders, first column is number of bytes\n");
#endif // ALTIVEC

  for (int l = 8; l <= N; l++) {
    printf("%d ", l);
    char *code = malloc(chromium_base64_encode_len(l));
    int codedlen = chromium_base64_encode(code, randombuffer, l);
    testdecode(code, codedlen, false);
    free(code);
    printf("\n");
  }

  if (freopen(encodingfilename, "w", stdout) == NULL) {
    printf("error opening %s \n", encodingfilename);
  }

#ifdef HAVE_AVX2
#ifndef HAVE_AVX512BW
  printf("#displaying cycles per input bytes for memcpy and linux, quicktime, chromium, scalar, "
         "avx2 (Klomp), avx2 encoders, first column is number of bytes\n");
#endif // do not HAVE_AVX512BW
#endif // HAVE_AVX2
#ifdef HAVE_AVX512BW
  printf("#displaying cycles per input bytes for memcpy and linux, quicktime, chromium, scalar, "
         "avx2 (Klomp), avx2, avx512 encoders, first column is number of bytes\n");
#endif // HAVE_AVX512BW
#ifdef ALTIVEC
  printf("#displaying nanoseconds per input bytes for memcpy and linux, quicktime, chromium, scalar, "
         "altivec encoders, first column is number of bytes\n");
#endif // ALTIVEC

  for (int l = 8; l <= N; l++) {
    printf("%d ", l);
    testencode(randombuffer, l, false);
    printf("\n");
  }
  const char *ttystr = "/dev/tty";
  if (freopen(ttystr, "w", stdout) == NULL) {
    printf("error opening %s \n", ttystr);
  }

  printf("Testing with real data.\n");

  printf("lena [jpg]\n");
  testdecode(lenabase64, sizeof(lenabase64) - 1, true);
  printf("peppers [jpg]\n");
  testdecode(peppersbase64, sizeof(peppersbase64) - 1, true);
  printf("mandril [jpg]\n");
  testdecode(mandrilbase64, sizeof(mandrilbase64) - 1, true);
  printf("moby_dick [text]\n");
  testdecode(moby_dick_base64, sizeof(moby_dick_base64) - 1, true);
  printf("google logo [png]\n");
  testdecode(googlelogo_base64, sizeof(googlelogo_base64) - 1, true);
  printf("bing.com social icons [png]\n");
  testdecode(bingsocialicon_base64, sizeof(bingsocialicon_base64) - 1, true);
  return 0;
}
