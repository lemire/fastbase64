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
           ./include/fastavxbase64.h \
           ./include/fastavx512bwbase64.h
OBJECTS += klompavxbase64.o \
           fastavxbase64.o \
           fastavx512bwbase64.o
endif # AVX2

ifeq ($(SIMD), AVX512BW)
CFLAGS += -march=native -mavx512bw -DHAVE_AVX512BW
endif # AVX512BW

ifeq ($(SIMD), ALTIVEC)
CFLAGS += -mcpu=power9 -mtune=power9 -flax-vector-conversions -DALTIVEC
HEADERS += ./include/altivecbase64.h
OBJECTS += altivecbase64.o
endif # AltiVec

%.o: ./src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -Iinclude

# basic_benchmark: ./my_benchmark/basic_benchmark.c  ./my_benchmark/benchmark.h  $(HEADERS) $(OBJECTS)
	# $(CC) $(CFLAGS) -o $@ ./my_benchmark/basic_benchmark.c -Iinclude  $(OBJECTS)
basic_benchmark: ./benchmarks/basic_benchmark.c  ./benchmarks/benchmark.h  $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ ./benchmarks/basic_benchmark.c -Iinclude  $(OBJECTS)

unit: ./tests/unit.c  $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o unit ./tests/unit.c -Iinclude  $(OBJECTS)

clean:
	rm -f unit basic_benchmark *.o
