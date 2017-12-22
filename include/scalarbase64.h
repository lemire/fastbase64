#ifndef SCALAR_B64
#define SCALAR_B64

/**
* Assumes recent x64 hardware with AVX2 instructions.
*/

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
* We copy the code below from https://github.com/aklomp/base64
**/


/* Wrapper function to decode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 3/4 the
 * size of the input. */
int scalar_base64_decode
	( const char		*src
	, size_t		 srclen
	, char			*out
	, size_t		*outlen
	) ;


/* Wrapper function to encode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 4/3 the
 * size of the input.  */
void scalar_base64_encode
	( const char		*src
	, size_t		 srclen
	, char			*out
	, size_t		*outlen
	) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
