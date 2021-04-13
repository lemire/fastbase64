.SUFFIXES:
.PHONY: all clean
.SUFFIXES: .cpp .o .c .h

all: unit basic_benchmark

CFLAGS = -fPIC -std=c99 -Wall -Wextra -Wshadow -Wpsabi -Wfatal-errors

HEADERS=./include/chromiumbase64.h \
        ./include/quicktimebase64.h \
        ./include/scalarbase64.h

OBJECTS=chromiumbase64.o \
        quicktimebase64.o \
        scalarbase64.o

SIMD ?= AVX2

ifeq ($(DEBUG), 1)
CFLAGS += -ggdb -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=address
else
CFLAGS += -O3
endif # debug

ifeq ($(SIMD), AVX2)
CFLAGS += -march=native -mavx2 -DHAVE_AVX2
HEADERS += ./include/klompavxbase64.h \
           ./include/fastavxbase64.h
OBJECTS += klompavxbase64.o \
           fastavxbase64.o
endif # AVX2

ifeq ($(SIMD), AVX512BW)
CFLAGS += -march=native -mavx512bw -DHAVE_AVX2 -DHAVE_AVX512BW
HEADERS += ./include/klompavxbase64.h \
           ./include/fastavxbase64.h \
           ./include/fastavx512bwbase64.h
OBJECTS += klompavxbase64.o \
           fastavxbase64.o \
           fastavx512bwbase64.o
endif # AVX512BW

ifeq ($(SIMD), ALTIVEC)
# -D_POSIX_C_SOURCE=199309L is needed to enable clock_gettime
CFLAGS += -mcpu=power9 -mtune=power9 -flax-vector-conversions -D_POSIX_C_SOURCE=199309L -DALTIVEC
HEADERS += ./include/altivecbase64.h
OBJECTS += altivecbase64.o
endif # AltiVec

%.o: ./src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -Iinclude

basic_benchmark: ./benchmarks/basic_benchmark.c  ./benchmarks/benchmark.h  $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ ./benchmarks/basic_benchmark.c -Iinclude  $(OBJECTS)

unit: ./tests/unit.c  $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o unit ./tests/unit.c -Iinclude  $(OBJECTS)

clean:
	rm -f unit basic_benchmark *.o
