#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#include "chromiumbase64.h"
#include "avxbase64.h"
#include "experimentalavxbase64.h"

#include "scalarbase64.h"

#include "quicktimebase64.h"
#include "linuxbase64.h"

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


typedef size_t (*DecodeFun)(char *out, const char *src, size_t srclen);


void test(const char* name, DecodeFun decode, struct Data* binary, struct Data* base64, struct Data* base64ws) {
   
    printf("testing %s... ", name);
    fflush(stdout);

    char* decoded = (char*)malloc(binary->size);

    decode(decoded, base64->data, base64->size);
    if (memcmp(decoded, binary->data, binary->size) != 0) {
        puts("FAILED (1)");
        return;
    }

    decode(decoded, base64ws->data, base64ws->size);
    if (memcmp(decoded, binary->data, binary->size) != 0) {
        puts("FAILED (2)");
        return;
    }

    puts("OK");
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        puts("usage: prog file.bin file.base64 file.base64-with-ws");
        return EXIT_FAILURE;
    }

    struct Data binary;
    struct Data base64;
    struct Data base64ws;

    load_file(argv[1], &binary);
    load_file(argv[2], &base64);
    load_file(argv[3], &base64ws);

    test("chrome", chromium_base64_decode, &binary, &base64, &base64ws);

    destroy(&binary);
    destroy(&base64);
    destroy(&base64ws);
}
