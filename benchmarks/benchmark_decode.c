#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include "benchmark.h"
#include "images_base64.h"
#include "moby_dick.h"
#include "webexamples.h"

#include "avxbase64.h"
#include "experimentalavxbase64.h"
#include "scalarbase64.h"
#include "chromiumbase64.h"
#include "quicktimebase64.h"
#include "linuxbase64.h"

static const int repeat = 50;

void testdecode(const char * data, size_t datalength, bool verbose) {
  if(verbose) printf("decoding a base64 input of  %zu bytes, ",datalength);
  char * buffer   = malloc(datalength * 2); // we allocate plenty of memory
  size_t expected = chromium_base64_decode(buffer, data,  datalength);
  if(verbose) printf("original size = %zu \n",expected);
  BEST_TIME_NOCHECK(memcpy(buffer, data, datalength),  , repeat, datalength,verbose);
 
  size_t outputlength;
  BEST_TIME_NOCHECK(expavx2_base64_decode(buffer, data, datalength), , repeat, datalength,verbose);

  free(buffer);
  if(verbose) printf("\n");
}


struct Data {
    char*   data;
    size_t  size;
};


void destroy(struct Data* d) {
    free(d->data);
    d->data = NULL;
}


void load_file(const char* path, struct Data* d) {

    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        printf("fopen: %s\n", strerror(errno));
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    d->size = ftell(f);
    d->data = malloc(d->size);

    fseek(f, 0, SEEK_SET);
    fread(d->data, 1, d->size, f);
    fclose(f);
}


int main(int argc, char **argv) {
  RDTSC_SET_OVERHEAD(rdtsc_overhead_func(1), repeat);

  if (argc == 1) {
    puts("usage: program base64-encoded-files");
    return EXIT_FAILURE;
  }

  struct Data base64;

  for (int i=1; i < argc; i++) {
      load_file(argv[i], &base64);

      printf("%s\n", argv[i]);
      testdecode(base64.data, base64.size, true);
      destroy(&base64);
  }

  return EXIT_SUCCESS;
}
