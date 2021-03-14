#ifndef EXPAVX_B64
#define EXPAVX_B64

#include "chromiumbase64.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
fastavxbase64 ported to AltiVec SIMD instruction set (Power architecture)
*/

size_t altivec_base64_decode(char *out, const char *src, size_t srclen);

size_t altivec_base64_encode(char *dest, const char *str, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
