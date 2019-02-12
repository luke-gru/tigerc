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
LEX=flex
YACC=bison
COMMON_SRCS = util.c errormsg.c
DEBUG_FLAGS=-O2 -g -rdynamic
#RELEASE_FLAGS=-O3 -DNDEBUG -Wno-unused-function
BUILD_DIR=bin
BUILD_FILE_LEXER_DEBUG=tigerc_lex
BUILD_FILE_INTERPRETER_DEBUG=tigerc_interp
BUILD_FILE_PARSER_DEBUG=tigerc_parse

# default
.PHONY: lexer
lexer: build lex.yy.o
	${CC} ${CFLAGS} $(COMMON_SRCS) nodes.c lex_driver.c lex.yy.o ${DEBUG_FLAGS} -o ${BUILD_DIR}/${BUILD_FILE_LEXER_DEBUG}

lex.yy.c: tiger.l
	${LEX} tiger.l

lex.yy.o: lex.yy.c tokens.h errormsg.h util.h
	${CC} -O2 -g -c lex.yy.c

y.tab.c: parse.y
	${YACC} -dv parse.y

y.tab.o: y.tab.c
	${CC} -O2 -g -c y.tab.c

.PHONY: interpreter
interpreter: build lex.yy.o
	${CC} ${CFLAGS} $(COMMON_SRCS) nodes.c interpreter.c interpreter_driver.c lex.yy.o ${DEBUG_FLAGS} -o ${BUILD_DIR}/${BUILD_FILE_INTERPRETER_DEBUG}

.PHONY: parser
parser: build y.tab.o lex.yy.o
	${CC} ${CFLAGS} $(COMMON_SRCS) symbol.c table.c ast.c parser_driver.c y.tab.o lex.yy.o ${DEBUG_FLAGS} -o ${BUILD_DIR}/${BUILD_FILE_PARSER_DEBUG}

.PHONY: build
build:
	mkdir -p ${BUILD_DIR}

.PHONY: clean
clean:
	rm -rf ${BUILD_DIR}
	rm -f *.o
	rm lex.yy.c
	rm y.tab.c
	rm y.tab.h
	rm y.output

