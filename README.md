# fastbase64

## Purpose

This code is part of a *research project*.
This is *not* a end-user library. Please don't use this code
as-is in your projects. There are better alternatives
if you want to accelerate base64 decoding in actual project.


## Story


We are investigating the possibility of SIMD-accelerated base64 codecs. We are motivated by earlier work by Muła  (https://github.com/WojciechMula/base64simd). This work was later used in a production-ready library (https://github.com/aklomp/base64).


## Results

We compare the AVX2 decoder lifted from the base64 library (https://github.com/aklomp/base64) with alternative. One particularly fast decoder is used by Google Chrome (and available in Chromium). Chromium uses code produced by Nick Galbreath  called "high performance base64 encoder / decoder".

Let us look at real data (images and text):

```
Testing with real data.
lena [jpg]
decoding a base64 input of  141020 bytes, original size = 105764
linux_base64_decode(buffer, data, data + datalength)        	:  19.81 cycles per operation (best) 	19.83 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                       	:  2.87 cycles per operation (best) 	2.87 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)            	:  1.85 cycles per operation (best) 	1.85 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)  	:  2.01 cycles per operation (best) 	2.01 cycles per operation (avg)
avx2_base64_decode(data,datalength,buffer,&outputlength)    	:  0.43 cycles per operation (best) 	0.43 cycles per operation (avg)

peppers [jpg]
decoding a base64 input of  12640 bytes, original size = 9478
linux_base64_decode(buffer, data, data + datalength)        	:  17.23 cycles per operation (best) 	18.39 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                       	:  2.86 cycles per operation (best) 	2.87 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)            	:  1.83 cycles per operation (best) 	1.84 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)  	:  2.02 cycles per operation (best) 	2.03 cycles per operation (avg)
avx2_base64_decode(data,datalength,buffer,&outputlength)    	:  0.44 cycles per operation (best) 	0.44 cycles per operation (avg)

mandril [jpg]
decoding a base64 input of  329632 bytes, original size = 247222
linux_base64_decode(buffer, data, data + datalength)        	:  20.09 cycles per operation (best) 	20.11 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                       	:  2.87 cycles per operation (best) 	2.87 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)            	:  1.85 cycles per operation (best) 	1.85 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)  	:  2.01 cycles per operation (best) 	2.02 cycles per operation (avg)
avx2_base64_decode(data,datalength,buffer,&outputlength)    	:  0.43 cycles per operation (best) 	0.45 cycles per operation (avg)

moby_dick [text]
decoding a base64 input of  1484 bytes, original size = 1111
linux_base64_decode(buffer, data, data + datalength)        	:  5.24 cycles per operation (best) 	9.42 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                       	:  2.89 cycles per operation (best) 	2.93 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)            	:  1.82 cycles per operation (best) 	1.84 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)  	:  2.05 cycles per operation (best) 	2.10 cycles per operation (avg)
avx2_base64_decode(data,datalength,buffer,&outputlength)    	:  0.55 cycles per operation (best) 	0.60 cycles per operation (avg)
```

Next plot shows results using random data of varying size:

[results/skylake_cyclesperinputbyte.png](results/skylake_cyclesperinputbyte.png)


## SIMD base64 decoding

Let us focus on decoding, the most performance-sensitive task.

### Character decoding (lookup)

Base64 writes 6-bit bytes in text form, not as byte values in [0,64). It is useful to take the text input and convert it to values in [0,64) if we want to decode base64 text. (This is not a necessary step however: some high performance base64 decoders do not include such a separate step, decoding base64 in one pass instead.) Muła calls this a lookup, possibly because it is commonly solved using a lookup table.

Muła showed (https://github.com/WojciechMula/base64simd) that you could quickly take a 32-byte vector of base64 encoded text and convert it to an array of integers in [0,64) using shifts, bitwise logical operations and shuffles. It is fast, but the caveat is that it does not do error checking and end-of-stream processing.

### Bit packing

Given the byte values in [0,64), i.e., 6-bit values, we must then pack them to finish the decoding. Base64 works by packing 4 bytes into 3 bytes as follows. The normal 4-byte to 3-byte base64 decoding routine goes as follows...

```
output[0] =  ( input[0] << 2 ) | ( input[1] >> 4)
output[1] =  ( input[1] << 4 ) | ( input[2] >> 2)
output[2] =  ( input[3] << 6 ) |  input[3]
```

See https://en.wikipedia.org/wiki/Base64#Sample_Implementation_in_Java for a reference implementation.

(Base64 decoders such as the one in the Chromium code base avoid shifts entirely by looking up bytes as "pre-shifted" 32-bit values.)


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

This almost correct, but base64 works in big endian mode so proper byte shuffling is needed.
