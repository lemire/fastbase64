# fastbase64


## Story

We are investigating the possibility of SIMD-accelerated base64 codecs. 

* Wojciech Muła, Daniel Lemire, Faster Base64 Encoding and Decoding Using AVX2 Instructions, ACM Transactions on the Web (to appear) https://arxiv.org/abs/1704.00605

Our AVX2 decoder does not use floating-point numbers.

## Sample usage


We extend's Nick Galbreath's base64 library (this high-performance library is used in Chromium).

We assume that you have an AVX2-capable machine (recent Intel processor from 2013 and up). In practice,
which function is called should be determined based on the underlying hardware.

### Encoding

Original encoding...
```
 char* src = ...;
 int srclen = ...; //the length of number of bytes in src
 char* dest = (char*) malloc(chromium_base64_decode_len(srclen)); // decode_len effectively multiplies by 4/3
 int len = chromium_base64_encode(dest, src, sourcelen); // returns how many bytes were decoded.
```
Encoding with AVX2...
```
 char* src = ...;
 int srclen = ...; //the length of number of bytes in src
 char* dest = (char*) malloc(chromium_base64_decode_len(srclen));
 int len = fast_avx2_base64_encode(dest, src, sourcelen); // returns how many bytes were decoded.
```

### Decoding

Original decoding...
```
char* src = ...;
int srclen = ...; // or if you don't know use strlen(src)
char* dest = (char*) malloc(chromium_base64_encode_len(srclen)); // effectively multiplies by 3/4
int len = chromium_base64_decode(dest, src, sourcelen);
if (len == MODP_B64_ERROR) { error }
```



Decoding with AVX2...

```
char* src = ...;
int srclen = ...; // or if you don't know use strlen(src)
char* dest = (char*) malloc(chromium_base64_encode_len(srclen)); // effectively multiplies by 3/4
int len = fast_avx2_base64_decode(dest, src, sourcelen);
if (len == MODP_B64_ERROR) { error }
```





## Results

We compare SIMD decoding with competitive alternatives.  One particularly fast decoder is used by Google Chrome (and available in Chromium). Chromium uses code produced by Nick Galbreath  called "high performance base64 encoder / decoder". We also use the decoder found in the Linux kernel as well as the one found in the QuickTime code (which was derived from code from the Apache HTTP server).

Let us look at real data (images and text):

```
$ ./basic_benchmark
rdtsc_overhead set to 30
Testing first with random data.
See files encodingperf.txt decodingperf.txt ...
Testing with real data.
lena [jpg]
decoding a base64 input of  141020 bytes, original size = 105764
memcpy(buffer, data, datalength)                                :  0.09 cycles per operation (best)     0.09 cycles per operation (avg)
linux_base64_decode(buffer, data, data + datalength)            :  19.73 cycles per operation (best)     19.79 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                           :  3.10 cycles per operation (best)     3.17 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)                :  1.84 cycles per operation (best)     1.84 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)      :  2.07 cycles per operation (best)     2.09 cycles per operation (avg)
klomp_avx2_base64_decode(data,datalength,buffer,&outputlength)    :  0.43 cycles per operation (best)     0.44 cycles per operation (avg)
fast_avx2_base64_decode(buffer, data, datalength)               :  0.21 cycles per operation (best)     0.21 cycles per operation (avg)

peppers [jpg]
decoding a base64 input of  12640 bytes, original size = 9478
memcpy(buffer, data, datalength)                                :  0.03 cycles per operation (best)     0.04 cycles per operation (avg)
linux_base64_decode(buffer, data, data + datalength)            :  15.05 cycles per operation (best)     15.94 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                           :  3.15 cycles per operation (best)     3.17 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)                :  1.82 cycles per operation (best)     1.82 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)      :  2.08 cycles per operation (best)     2.09 cycles per operation (avg)
klomp_avx2_base64_decode(data,datalength,buffer,&outputlength)    :  0.44 cycles per operation (best)     0.44 cycles per operation (avg)
fast_avx2_base64_decode(buffer, data, datalength)               :  0.21 cycles per operation (best)     0.21 cycles per operation (avg)

mandril [jpg]
decoding a base64 input of  329632 bytes, original size = 247222
memcpy(buffer, data, datalength)                                :  0.11 cycles per operation (best)     0.12 cycles per operation (avg)
linux_base64_decode(buffer, data, data + datalength)            :  20.02 cycles per operation (best)     20.08 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                           :  3.10 cycles per operation (best)     3.17 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)                :  1.84 cycles per operation (best)     1.84 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)      :  2.07 cycles per operation (best)     2.08 cycles per operation (avg)
klomp_avx2_base64_decode(data,datalength,buffer,&outputlength)    :  0.43 cycles per operation (best)     0.44 cycles per operation (avg)
fast_avx2_base64_decode(buffer, data, datalength)               :  0.22 cycles per operation (best)     0.22 cycles per operation (avg)

moby_dick [text]
decoding a base64 input of  1484 bytes, original size = 1111
memcpy(buffer, data, datalength)                                :  0.04 cycles per operation (best)     0.05 cycles per operation (avg)
linux_base64_decode(buffer, data, data + datalength)            :  3.61 cycles per operation (best)     4.33 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                           :  3.20 cycles per operation (best)     3.21 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)                :  1.83 cycles per operation (best)     1.83 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)      :  2.11 cycles per operation (best)     2.13 cycles per operation (avg)
klomp_avx2_base64_decode(data,datalength,buffer,&outputlength)    :  0.53 cycles per operation (best)     0.54 cycles per operation (avg)
fast_avx2_base64_decode(buffer, data, datalength)               :  0.28 cycles per operation (best)     0.29 cycles per operation (avg)

google logo [png]
decoding a base64 input of  3144 bytes, original size = 2357
memcpy(buffer, data, datalength)                                :  0.05 cycles per operation (best)     0.05 cycles per operation (avg)
linux_base64_decode(buffer, data, data + datalength)            :  4.36 cycles per operation (best)     6.37 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                           :  3.16 cycles per operation (best)     3.18 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)                :  1.81 cycles per operation (best)     1.82 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)      :  2.09 cycles per operation (best)     2.10 cycles per operation (avg)
klomp_avx2_base64_decode(data,datalength,buffer,&outputlength)    :  0.47 cycles per operation (best)     0.47 cycles per operation (avg)
fast_avx2_base64_decode(buffer, data, datalength)               :  0.23 cycles per operation (best)     0.24 cycles per operation (avg)

bing.com social icons [png]
decoding a base64 input of  1808 bytes, original size = 1355
memcpy(buffer, data, datalength)                                :  0.04 cycles per operation (best)     0.04 cycles per operation (avg)
linux_base64_decode(buffer, data, data + datalength)            :  3.97 cycles per operation (best)     5.25 cycles per operation (avg)
quicktime_base64_decode(buffer, data)                           :  3.13 cycles per operation (best)     3.20 cycles per operation (avg)
chromium_base64_decode(buffer, data, datalength)                :  1.82 cycles per operation (best)     1.83 cycles per operation (avg)
scalar_base64_decode(data,datalength,buffer,&outputlength)      :  2.11 cycles per operation (best)     2.27 cycles per operation (avg)
klomp_avx2_base64_decode(data,datalength,buffer,&outputlength)    :  0.46 cycles per operation (best)     0.47 cycles per operation (avg)
fast_avx2_base64_decode(buffer, data, datalength)               :  0.24 cycles per operation (best)     0.24 cycles per operation (avg)
```

Next plot shows results using random data of varying size:

<img src="https://github.com/lemire/fastbase64/blob/master/results/skylake_decoding_cyclesperinputbyte.png" width="50%" />

We see that for base64 inputs of 100 bytes or more the AVX2 decoder is much faster, being more than three times faster.


## How does SIMD base64 decoding works?

Let us focus on decoding, the most performance-sensitive task.

### Character decoding (lookup)

Base64 writes 6-bit bytes in text form, not as byte values in [0,64). It is useful to take the text input and convert it to values in [0,64) if we want to decode base64 text. (This is not a necessary step however: some high performance base64 decoders do not include such a separate step, decoding base64 in one pass instead.) Muła calls this a lookup, possibly because it is commonly solved using a lookup table.

Muła showed (https://github.com/WojciechMula/base64simd) that you could quickly take a 32-byte vector of base64 encoded text and convert it to an array of integers in [0,64) using shifts, bitwise logical operations and shuffles. It is fast.

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
