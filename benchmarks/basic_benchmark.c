#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "benchmark.h"
#include "images_base64.h"
#include "moby_dick.h"

#include "avxbase64.h"
#include "experimentalavxbase64.h"
#include "scalarbase64.h"
#include "chromiumbase64.h"
#include "quicktimebase64.h"
#include "linuxbase64.h"

static const int repeat = 5;

void testdecode(const char * data, size_t datalength, bool verbose) {
  if(verbose) printf("decoding a base64 input of  %zu bytes, ",datalength);
  if (datalength < 4 || (datalength % 4 != 0)) {
    printf("size should be divisible by 4 bytes.\n");
    return;
  }
  char * buffer = malloc(datalength * 2); // we allocate plenty of memory
  size_t expected =   chromium_base64_decode(buffer, data,  datalength);
  if(verbose) printf("original size = %zu \n",expected);

  BEST_TIME(linux_base64_decode(buffer, data, data + datalength), (int) expected, , repeat, datalength,verbose);
  BEST_TIME(quicktime_base64_decode(buffer, data), (int) expected, , repeat, datalength,verbose);
  BEST_TIME(chromium_base64_decode(buffer, data, datalength), (int) expected, , repeat, datalength,verbose);

  size_t outputlength;
  int avxexpected =  avx2_base64_decode(data,datalength,buffer,&outputlength);
  assert(outputlength == expected);
  BEST_TIME(scalar_base64_decode(data,datalength,buffer,&outputlength), avxexpected, , repeat, datalength,verbose);
  BEST_TIME(avx2_base64_decode(data,datalength,buffer,&outputlength), avxexpected, , repeat, datalength,verbose);
  BEST_TIME(expavx2_base64_decode(data,datalength,buffer,&outputlength), avxexpected, , repeat, datalength,verbose);

  free(buffer);
  if(verbose) printf("\n");
}

int main() {
//int main(int argc, char **argv) {
  RDTSC_SET_OVERHEAD(rdtsc_overhead_func(1), repeat);

  printf("Testing first with random data.\n");
  const int N = 1024;
  char randombuffer[N];
  for(int k = 0; k < N; ++k) randombuffer[k] = rand();
  printf("displaying cycles per input bytes for linux, quicktime, chromium, scalar and avx2 decoders, first column is number of bytes\n");

  for(int l = 8; l <= N; l += 4) {
    printf("%d ",l);
    char * code = (char*) malloc(chromium_base64_encode_len(l));
    int codedlen = chromium_base64_encode(code, randombuffer, l);
    testdecode(code, codedlen, false);
    free(code);
    printf("\n");

  }


  printf("Testing with real data.\n");

  printf("lena [jpg]\n");
  testdecode(lenabase64, sizeof(lenabase64) - 1,true);
  printf("peppers [jpg]\n");
  testdecode(peppersbase64, sizeof(peppersbase64) - 1,true);
  printf("mandril [jpg]\n");
  testdecode(mandrilbase64, sizeof(mandrilbase64) - 1,true);
  printf("moby_dick [text]\n");
  testdecode(moby_dick_base64, sizeof(moby_dick_base64) - 1,true);

  return 0;
}
