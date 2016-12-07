#ifndef AVX_B64
#define AVX_B64

/**
* Assumes recent x64 hardware with AVX2 instructions.
*/

#include <stddef.h>
#include <stdint.h>

/**
* This function converts base64 text into an array of bytes (in [0,64))
* without support for "end of stream".
*
* from https://github.com/WojciechMula/base64simd
*/
__m256i lookup_pshufb(const __m256i input);

#endif
