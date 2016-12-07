# fastbase64

We are investigating the possibility of SIMD-accelerated base64 codecs. We are motivated by earlier work by Muła  (https://github.com/WojciechMula/base64simd). One concern is that Muła does not include unit tests that check that his code maps actual data to standard base64 outputs and back. We would like to not only derive a fast SIMD-based base64 code, but also show that it is correct using unit tests, with actual base64 examples (see https://github.com/lemire/fastbase64/blob/master/tests/unit.c). Muła's unit tests take a different form (https://github.com/WojciechMula/base64simd/blob/master/decode/verify.cpp) that may or may not prove correct base64 decoding.

## SIMD base64 decoding

Let us focus on decoding, the most performance-sensitive task.

### Character decoding (lookup)

Base64 writes 6-bit bytes in text form, not as byte values in [0,64). It is useful to take the text input and convert it to values in [0,64) if we want to decode base64 text. (This is not a necessary step however: some high performance base64 decoders do not include such a separate step, decoding base64 in one pass instead.) Muła calls this a lookup, possibly because it is commonly solved using a lookup table.

Muła showed (https://github.com/WojciechMula/base64simd) that you could quickly take a 32-byte vector of base64 encoded text and convert it to an array of integers in [0,64) using shifts, bitwise logical operations and shuffles. It is fast, but the caveat is that it does not do error checking and end-of-stream processing. (See https://github.com/lemire/fastbase64/blob/master/src/avxbase64.c )

### Bit packing

Given the byte values in [0,64), i.e., 6-bit values, we must then pack them to finish the decoding. Base64 works by packing 4 bytes into 3 bytes as follows. The normal 4-byte to 3-byte base64 decoding routine goes as follows...

```
output[0] =  ( input[0] << 2 ) | ( input[1] >> 4)
output[1] =  ( input[1] << 4 ) | ( input[2] >> 2)
output[2] =  ( input[3] << 6 ) |  input[3]
```

See https://en.wikipedia.org/wiki/Base64#Sample_Implementation_in_Java for a reference implementation.

Other similar implementations can be find online:
- https://opensource.apple.com/source/QuickTimeStreamingServer/QuickTimeStreamingServer-452/CommonUtilitiesLib/base64.c

(Base64 decoders such as the one in the Chromium code base avoid shifts entirely by looking up bytes as "pre-shifted" 32-bit values.)


Suppose that ``input[1]`` is 63 (0b00111111), and all other input bytes are zero, what would be the result?

```
input[0] = 0b00000000
input[1] = 0b00111111
input[2] = 0b00000000
input[3] = 0b00000000
```
In little endian notation, that would be 0x3f00,
and we want to get...

```
output[0] =  0b00000011
output[1] =  0b11110000
output[2] =  0b00000000
```

In little endian notation, that would be 0x3f00 or 0b1111000000000011.

Note that it is not possible to go from 0b11111100000000 to 0b1111000000000011 using only a shift. However, we could get the desired result by a shift followed by a byte shuffle.

### Muła's bit packing

Muła addresses the issue of "gathering data" from the result of the lookup:
http://0x80.pl/notesen/2016-01-17-sse-base64-decoding.html#gathering-data


In a naive form, Muła suggests we use code as this :

```
const __m128i bits_a = _mm_and_si128(values, _mm256_set1_epi32(0x0000003f));
const __m128i bits_b = _mm_srli_epi32(_mm_and_si128(values, _mm256_set1_epi32(0x00003f00)), 2);
const __m128i bits_c = _mm_srli_epi32(_mm_and_si128(values, _mm256_set1_epi32(0x003f0000)), 4);
const __m128i bits_d = _mm_srli_epi32(_mm_and_si128(values, _mm256_set1_epi32(0x3f000000)), 6);

result = _mm_or_si128(bits_a, _mm_or_si128(bits_b, _mm_or_si128(bits_c, bits_d)));
```

Is this correct?

It would imply that we could pack 32-bit base64 words as follows:

```
(x & 0x0000003f) | ((x & 0x00003f00) >> 2)  | ((x & 0x003f0000) >> 4)  | ((x & 0x3f000000) >> 6)
```

Except that this function will not map 0b11111100000000 to 0b1111000000000011.

I think we need not only shifts but also byte shuffles.

Inside Mula's scalar decoding code, we find something as follows...

```
output[0] =  input[0] | ( input[1] << 6)
output[1] =  ( input[1] >> 2 ) | ( input[2] << 4 )
output[2] =  ( input[3] >> 4 ) | ( input[3] << 2 )
```

... which looks very similar to the standard approach, which is as follows...


```
output[0] =  ( input[0] << 2 ) | ( input[1] >> 4)
output[1] =  ( input[1] << 4 ) | ( input[2] >> 2)
output[2] =  ( input[3] << 6 ) |  input[3]
```

... except that the byte orders of both the input and the output are reversed.

## Conclusion?

Is Muła's approach with respect to gathering bits (bit packing) correct? If it is wrong, surely it can be fixed... though maybe at the cost of extra operations.
