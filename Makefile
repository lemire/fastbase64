# minimalist makefile
.SUFFIXES:
.PHONY: all clean
#
.SUFFIXES: .cpp .o .c .h
CFLAGS= -fPIC -std=c99 -Wall -Wextra -Wshadow -Wpsabi -Wfatal-errors
ifeq ($(DEBUG),1)
CFLAGS += -ggdb -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address
else
CFLAGS += -O3 -march=native -mavx2
endif # debug

all: unit basic_benchmark test_ws_decode


HEADERS=./include/chromiumbase64.h \
        ./include/avxbase64.h \
        ./include/quicktimebase64.h \
        ./include/scalarbase64.h \
        ./include/experimentalavxbase64.h

OBJECTS=chromiumbase64.o \
        avxbase64.o \
        experimentalavxbase64.o \
        quicktimebase64.o \
        scalarbase64.o 


%.o: ./src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -Iinclude

src/compress.inl: src/prepare_luts.py
	python $^ > $@

basic_benchmark: ./benchmarks/basic_benchmark.c  ./benchmarks/benchmark.h   src/compress.inl $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ ./benchmarks/basic_benchmark.c -Iinclude  $(OBJECTS)

unit: ./tests/unit.c src/compress.inl $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ ./tests/unit.c -Iinclude  $(OBJECTS)

test_ws_decode: ./tests/test_ws_decode.c src/compress.inl $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ ./tests/test_ws_decode.c -Iinclude  $(OBJECTS)

clean:
	rm -f unit basic_benchmark *.o
