#ifndef EXP2AVX_B64
#define EXP2AVX_B64

/**
* Assumes recent x64 hardware with AVX2 instructions.
*/

#include <stddef.h>
#include <stdint.h>


/**
* Part of this code is based on Alfred Klomp's https://github.com/aklomp/base64 (published under BSD)
**/


/* Wrapper function to decode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 3/4 the
 * size of the input.
 *
 * Returns a non-zero value on error:
 * 1: end-of-stream while waiting for the last '=' character
 * 2: invalid input
 */
int exp2avx2_base64_decode
	( const char		*src
	, size_t		 srclen
	, char			*out
	, size_t		*outlen
	) ;


/* Wrapper function to encode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 4/3 the
 * size of the input.  */
void exp2avx2_base64_encode
	( const char		*src
	, size_t		 srclen
	, char			*out
	, size_t		*outlen
	) ;


#endif
