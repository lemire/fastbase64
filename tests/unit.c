
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "chromiumbase64.h"
#include "avxbase64.h"

void checkExample(const char * source, const char * coded) {
  unsigned int len;
  unsigned int codedlen;

  char * dest1 = (char*) malloc(modp_b64_encode_len(strlen(source)));
  codedlen = modp_b64_encode(dest1, source, strlen(source));
  assert(codedlen >= 0);
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(modp_b64_decode_len(codedlen));
  len = modp_b64_decode(dest2, coded, codedlen);
  assert(len >= 0);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(modp_b64_decode_len(codedlen));
  len = modp_b64_decode(dest3, dest1, codedlen);
  assert(len >= 0);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}


void avxtest(const char * source, const char * coded) {
  unsigned int codedlen;

  char * dest1 = (char*) malloc(modp_b64_encode_len(strlen(source)));
  codedlen = modp_b64_encode(dest1, source, strlen(source));
  assert(codedlen >= 0);
  assert(strncmp(dest1,coded,codedlen) == 0);
  assert(codedlen>=32);
  char * recov = (char*) malloc(strlen(source) + 1);
  int recovlen = avx_b64_decode(recov,dest1,32);

  printf("%.*s", recovlen, recov);
  assert(strncmp(recov,source,recovlen) == 0);

  free(dest1);
  free(recov);
}

int main() {


  // from Wikipedia page
  const char * wikipediasource = "Man is distinguished, not only by his reason, but by this singular passion from \
other animals, which is a lust of the mind, that by a perseverance of delight \
in the continued and indefatigable generation of knowledge, exceeds the short \
vehemence of any carnal pleasure.";
  const char * wikipediacoded = "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz\
IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg\
dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu\
dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo\
ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=";

  // from https://gobyexample.com/base64-encoding
  const char * gosource = "abc123!?$*&()'-=@~";
  const char * gocoded = "YWJjMTIzIT8kKiYoKSctPUB+";

  // from https://www.tutorialspoint.com/java8/java8_base64.htm
  const char * tutosource = "TutorialsPoint?java8";
  const char * tutocoded = "VHV0b3JpYWxzUG9pbnQ/amF2YTg=";


  checkExample(wikipediasource,wikipediacoded);
  checkExample(gosource,gocoded);
  checkExample(tutosource,tutocoded);

  avxtest(wikipediasource,wikipediacoded);


	printf("Code looks ok.\n");
  return 0;
}
