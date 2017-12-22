#ifndef EXPAVX_B64
#define EXPAVX_B64

/**
* Assumes recent x64 hardware with AVX2 instructions.
*/

#include <stddef.h>
#include <stdint.h>
#include "chromiumbase64.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
* This code extends Nick Galbreath's high performance base 64decoder (used in Chromium), the API is the
* same effectively, see chromium64.h.
*/

/*
* AVX2 accelerated version of Galbreath's chromium_base64_decode function
* Usage remains the same, see chromium.h.
*/
size_t fast_avx2_base64_decode(char *out, const char *src, size_t srclen);

/*
* AVX2 accelerated version of Galbreath's chromium_base64_encode function
* Usage remains the same, see chromium.h.
*/
size_t fast_avx2_base64_encode(char* dest, const char* str, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
