#ifndef TURBOBASE64_B64
#define TURBOBASE64_B64


#include <stddef.h>
#include <stdint.h>

// https://github.com/powturbo/TurboBase64


/* Wrapper function to decode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 3/4 the
 * size of the input. */
int turbobase64_base64_decode
	( const char	*src
	, size_t		 srclen
	, char			*out
	) ;


/* Wrapper function to encode a plain string of given length. Output is written
 * to *out without trailing zero. Output length in bytes is written to *outlen.
 * The buffer in `out` has been allocated by the caller and is at least 4/3 the
 * size of the input.  */
void turbobase64_base64_encode
	( const char	*src
	, size_t		 srclen
	, char			*out
	) ;


#endif
