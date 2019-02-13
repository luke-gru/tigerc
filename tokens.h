#ifndef tiger_tokens_h
#define tiger_tokens_h

#include "util.h"

typedef union {
    int pos;
    int num;
    string str;
} YYSTYPE;

extern YYSTYPE yylval;

#define TK_ID 257
#define TK_STRING 258
#define TK_INT 259
#define TK_COMMA 260
#define TK_COLON 261
#define TK_SEMICOLON 262
#define TK_LPAREN 263
#define TK_RPAREN 264
#define TK_LBRACK 265
#define TK_RBRACK 266
#define TK_LBRACE 267
#define TK_RBRACE 268
#define TK_DOT 269
#define TK_PLUS 270
#define TK_MINUS 271
#define TK_TIMES 272
#define TK_DIVIDE 273
#define TK_EQ 274
#define TK_NEQ 275
#define TK_LT 276
#define TK_LE 277
#define TK_GT 278
#define TK_GE 279
#define TK_AND 280
#define TK_OR 281
#define TK_ASSIGN 282
#define TK_ARRAY 283
#define TK_IF 284
#define TK_THEN 285
#define TK_ELSE 286
#define TK_WHILE 287
#define TK_FOR 288
#define TK_TO 289
#define TK_DO 290
#define TK_LET 291
#define TK_IN 292
#define TK_END 293
#define TK_OF 294
#define TK_BREAK 295
#define TK_NIL 296
#define TK_FUNCTION 297
#define TK_VAR 298
#define TK_TYPE 299

static string toknames[] = {
    "ID", "STRING", "INT", "COMMA", "COLON", "SEMICOLON", "LPAREN",
    "RPAREN", "LBRACK", "RBRACK", "LBRACE", "RBRACE", "DOT", "PLUS",
    "MINUS", "TIMES", "DIVIDE", "EQ", "NEQ", "LT", "LE", "GT", "GE",
    "AND", "OR", "ASSIGN", "ARRAY", "IF", "THEN", "ELSE", "WHILE", "FOR",
    "TO", "DO", "LET", "IN", "END", "OF", "BREAK", "NIL", "FUNCTION",
    "VAR", "TYPE"
};

static inline string tokname(int tok) {
  return tok<257 || tok>299 ? "BAD_TOKEN" : toknames[tok-257];
}

#endif
