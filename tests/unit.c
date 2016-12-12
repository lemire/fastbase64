
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "chromiumbase64.h"
#include "avxbase64.h"
#include "experimentalavxbase64.h"
#include "experimental2avxbase64.h"

#include "scalarbase64.h"

#include "quicktimebase64.h"
#include "linuxbase64.h"


void chromium_checkExample(const char * source, const char * coded) {
  printf("chromium codec check.\n");
  unsigned int len;
  unsigned int codedlen;

  char * dest1 = (char*) malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = chromium_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = chromium_base64_decode(dest2, coded, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = chromium_base64_decode(dest3, dest1, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}


void quicktime_checkExample(const char * source, const char * coded) {
  printf("quicktime codec check.\n");
  unsigned int len;
  unsigned int codedlen;

  char * dest1 = (char*) malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = quicktime_base64_encode(dest1, source, strlen(source));
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = quicktime_base64_decode(dest2, coded);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = quicktime_base64_decode(dest3, dest1);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}


void linux_checkExample(const char * source, const char * coded) {
  printf("linux codec check.\n");
  unsigned int len;
  unsigned int codedlen;

  char * dest1 = (char*) malloc(chromium_base64_encode_len(strlen(source)));
  codedlen = chromium_base64_encode(dest1, source, strlen(source));
  //codedlen = linux_base64_encode(dest1, source, source + strlen(source));
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = linux_base64_decode(dest2, coded, coded + codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = linux_base64_decode(dest3, dest1, dest1 + codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}


void avx2_checkExample(const char * source, const char * coded) {
  printf("avx2 codec check.\n");
  size_t len;
  size_t codedlen;

  char * dest1 = (char*) malloc(chromium_base64_encode_len(strlen(source)));

  avx2_base64_encode(source, strlen(source),dest1,&codedlen);
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(chromium_base64_decode_len(codedlen));
  avx2_base64_decode(coded,codedlen,dest2,&len);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(chromium_base64_decode_len(codedlen));
  avx2_base64_decode(dest1,codedlen,dest3,&len);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}


void expavx2_checkExample(const char * source, const char * coded) {
  printf("experimentalavx2 codec check.\n");
  size_t len;
  size_t codedlen;

  char * dest1 = (char*) malloc(chromium_base64_encode_len(strlen(source)));

  avx2_base64_encode(source, strlen(source),dest1,&codedlen);
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = expavx2_base64_decode(dest2, coded, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(chromium_base64_decode_len(codedlen));
  len = expavx2_base64_decode(dest3, dest1, codedlen);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}



void exp2avx2_checkExample(const char * source, const char * coded) {
  printf("experimental2avx2 codec check.\n");
  size_t len;
  size_t codedlen;

  char * dest1 = (char*) malloc(chromium_base64_encode_len(strlen(source)));

  exp2avx2_base64_encode(source, strlen(source),dest1,&codedlen);
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(chromium_base64_decode_len(codedlen));
  exp2avx2_base64_decode(coded,codedlen,dest2,&len);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(chromium_base64_decode_len(codedlen));
  exp2avx2_base64_decode(dest1,codedlen,dest3,&len);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
}



void scalar_checkExample(const char * source, const char * coded) {
  printf("scalar codec check.\n");
  size_t len;
  size_t codedlen;

  char * dest1 = (char*) malloc(chromium_base64_encode_len(strlen(source)));

  scalar_base64_encode(source, strlen(source),dest1,&codedlen);
  assert(strncmp(dest1,coded,codedlen) == 0);
  char *dest2 = (char*) malloc(chromium_base64_decode_len(codedlen));
  scalar_base64_decode(coded,codedlen,dest2,&len);
  assert(len == strlen(source));
  assert(strncmp(dest2,source,strlen(source)) == 0);
  char *dest3 = (char*) malloc(chromium_base64_decode_len(codedlen));
  scalar_base64_decode(dest1,codedlen,dest3,&len);
  assert(len == strlen(source));
  assert(strncmp(dest3,source,strlen(source)) == 0);
  free(dest1);
  free(dest2);
  free(dest3);
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


  chromium_checkExample(wikipediasource,wikipediacoded);
  chromium_checkExample(gosource,gocoded);
  chromium_checkExample(tutosource,tutocoded);

  avx2_checkExample(wikipediasource,wikipediacoded);
  avx2_checkExample(gosource,gocoded);
  avx2_checkExample(tutosource,tutocoded);

  expavx2_checkExample(wikipediasource,wikipediacoded);
  expavx2_checkExample(gosource,gocoded);
  expavx2_checkExample(tutosource,tutocoded);

  exp2avx2_checkExample(wikipediasource,wikipediacoded);
  exp2avx2_checkExample(gosource,gocoded);
  exp2avx2_checkExample(tutosource,tutocoded);

  scalar_checkExample(wikipediasource,wikipediacoded);
  scalar_checkExample(gosource,gocoded);
  scalar_checkExample(tutosource,tutocoded);

  quicktime_checkExample(wikipediasource,wikipediacoded);
  quicktime_checkExample(gosource,gocoded);
  quicktime_checkExample(tutosource,tutocoded);

  linux_checkExample(wikipediasource,wikipediacoded);
  linux_checkExample(gosource,gocoded);
  linux_checkExample(tutosource,tutocoded);


	printf("Code looks ok.\n");
  return 0;
}
