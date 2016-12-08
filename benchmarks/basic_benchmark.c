#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "benchmark.h"
#include "images_base64.h"

#include "avxbase64.h"
#include "chromiumbase64.h"
#include "quicktimebase64.h"
#include "linuxbase64.h"


void testdecode(const char * data, size_t datalength) {
  printf("decoding a base64 input of  %zu bytes, ",datalength);
  if (datalength < 4 || (datalength % 4 != 0)) {
    printf("size should be divisible by 4 bytes.\n");
    return;
  }
  char * buffer = malloc(datalength * 2); // we allocate plenty of memory
  size_t expected =   chromium_base64_decode(buffer, data,  datalength);
  printf("original size = %zu \n",expected);
  int repeat = 5;
  BEST_TIME(linux_base64_decode(buffer, data, data + datalength), (int) expected, , repeat, datalength);
  BEST_TIME(quicktime_base64_decode(buffer, data), (int) expected, , repeat, datalength);
  BEST_TIME(chromium_base64_decode(buffer, data, datalength), (int) expected, , repeat, datalength);

  size_t outputlength;
  int avxexpected =  avx2_base64_decode(data,datalength,buffer,&outputlength);
  assert(outputlength == expected);
  BEST_TIME(avx2_base64_decode(data,datalength,buffer,&outputlength), avxexpected, , repeat, datalength);
  free(buffer);
}

int main(int argc, char **argv) {
  testdecode(lenabase64, sizeof(lenabase64) - 1);
  testdecode(peppersbase64, sizeof(peppersbase64) - 1);
  testdecode(mandrilbase64, sizeof(mandrilbase64) - 1);
  return 0;
}
