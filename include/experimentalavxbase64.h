#ifndef EXPAVX_B64
#define EXPAVX_B64

/**
* Assumes recent x64 hardware with AVX2 instructions.
*/

#include <stddef.h>
#include <stdint.h>


/**
* We copy the code below from https://raw.githubusercontent.com/aklomp/base64
**/


size_t expavx2_base64_decode(char *out, const char *src, size_t srclen);


/* Wrapper function to encode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 4/3 the
 * size of the input.  */
void expavx2_base64_encode
	( const char		*src
	, size_t		 srclen
	, char			*out
	, size_t		*outlen
	) ;


#endif
