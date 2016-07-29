CC = gcc

SPN_HEADERS=/usr/local/include/spn
SPN_LIB=/usr/local/lib/libspn.a
LIBUV_HEADERS=/Users/artit91/projects/libuv/include
LIBUV_LIB=/Users/artit91/projects/libuv/build/Release/libuv.a

INCLUDES=-I$(SPN_HEADERS) -I$(LIBUV_HEADERS)

SRCDIR = src

LINKER_FLAGS = -Wl $(LIBUV_LIB)\
	-Wl $(SPN_LIB)\
	-dynamiclib\
	-undefined suppress\
	-flat_namespace

EXTRA_WARNINGS = -Wno-error=unused-function\
	-Wno-error=sign-compare\
	-Wno-error=logical-op-parentheses\
	-Wimplicit-fallthrough\
	-Wno-unused-parameter\
	-Wno-error-deprecated-declarations\
	-Wno-error=missing-field-initializers

WARNINGS = -Wall -Wextra -Werror $(EXTRA_WARNINGS)

CFLAGS = $(INCLUDES) -c -std=c89 -pedantic -fpic -fstrict-aliasing $(WARNINGS)

all:
	mkdir ./build
	$(CC) $(CFLAGS) $(SRCDIR)/loop.c  -o ./build/loop.o
	$(CC) $(CFLAGS) $(SRCDIR)/spnuv.c -o ./build/spnuv.o
	$(CC) $(CFLAGS) $(SRCDIR)/tcp.c -o ./build/tcp.o
	$(CC) $(LINKER_FLAGS) ./build/*.o -o build.dylib

clean:
	rm -rf ./build
	rm -rf build.dylib
