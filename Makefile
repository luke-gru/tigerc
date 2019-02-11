ifeq ($(strip $(CC)),)
CC=gcc
endif
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
DEFINES=-D_GNU_SOURCE
GCC_CFLAGS=-std=c99 -Wall -Wextra -Wmissing-prototypes -Wno-shadow -Wvla -Wno-clobbered -Wno-unused-parameter -Wno-unused-label -I. ${DEFINES}
GPP_CFLAGS=-std=c++11 -w -fpermissive -I. ${DEFINES}
# NOTE: clang++ doesn't compile yet, too many C++ type errors
CLANGPP_CFLAGS=-std=c++11 -w -fpermissive -I. ${DEFINES}
CLANG_CFLAGS=-std=c99 -Wall -Wextra -Wmissing-prototypes -I. -Wno-unused-parameter -Wno-unused-label ${DEFINES}
ifneq (,$(findstring clang,$(CC)))
	ifneq (,$(findstring clang++,$(CC)))
		CFLAGS=${CLANGPP_CFLAGS}
	else
		CFLAGS=${CLANG_CFLAGS}
	endif
else
  ifneq (,$(findstring g++,$(CC)))
    CFLAGS=${GPP_CFLAGS}
  else
    CFLAGS=${GCC_CFLAGS}
  endif
endif
SRCS = main.c util.c nodes.c interpreter.c
DEBUG_FLAGS=-O2 -g -rdynamic
RELEASE_FLAGS=-O3 -DNDEBUG -Wno-unused-function
BUILD_DIR=bin
BUILD_FILE_RELEASE=tigerc
BUILD_FILE_DEBUG=tigerc

# default
.PHONY: debug
debug: build
	${CC} ${CFLAGS} $(SRCS) ${DEBUG_FLAGS} -o ${BUILD_DIR}/${BUILD_FILE_DEBUG}

.PHONY: release
release: build
	${CC} ${CFLAGS} $(SRCS) ${RELEASE_FLAGS} -o ${BUILD_DIR}/${BUILD_FILE_RELEASE}

.PHONY: build
build:
	mkdir -p ${BUILD_DIR}

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR}
	rm -f *.o

