#ifndef AVX_B64
#define AVX_B64

/**
* Assumes recent x64 hardware with AVX2 instructions.
*/

#include <stddef.h>
#include <stdint.h>

/**
* "size" is the size of the input
* should, theoretically, return the number of written bytes
*/
size_t avx_b64_decode(char* out, const char* input, size_t size);

#endif
