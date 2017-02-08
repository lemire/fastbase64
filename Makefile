# minimalist makefile
.SUFFIXES:
.PHONY: all clean
#
.SUFFIXES: .cpp .o .c .h
CFLAGS= -fPIC -std=c99 -Wall -Wextra -Wshadow -Wpsabi
ifeq ($(DEBUG),1)
CFLAGS += -ggdb -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address
else
CFLAGS += -O3 -march=native -mavx2
endif # debug

all: unit basic_benchmark


HEADERS=./include/chromiumbase64.h \
        ./include/klompavxbase64.h \
        ./include/quicktimebase64.h \
        ./include/scalarbase64.h \
        ./include/fastavxbase64.h

OBJECTS=chromiumbase64.o \
        fastavxbase64.o \
        klompavxbase64.o \
        quicktimebase64.o \
        scalarbase64.o 


%.o: ./src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -Iinclude

basic_benchmark: ./benchmarks/basic_benchmark.c  ./benchmarks/benchmark.h  $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ ./benchmarks/basic_benchmark.c -Iinclude  $(OBJECTS)

unit: ./tests/unit.c  $(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -o unit ./tests/unit.c -Iinclude  $(OBJECTS)

clean:
	rm -f unit basic_benchmark *.o
