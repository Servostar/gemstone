
#ifndef LEX_UTIL_H_
#define LEX_UTIL_H_

#include <stdio.h>
#include <yacc/parser.tab.h>

#define MAX_READ_BUFFER_SIZE 1000

extern FILE* yyin;
extern YYLTYPE yylloc;
extern char* buffer;

/**
 * @brief Initialize global state needed for the lexer
 */
void lex_init(void);

void lex_reset(void);

/**
 * @brief Begin counting a new token. This will fill the global struct yylloc.
 * @param t the text of the token. Must be null terminated
 */
[[gnu::nonnull(1)]]
void beginToken(char* t);

/**
 * @brief Stores the next character into the supplied buffer
 * @param dst the buffer to store character in
 */
int nextChar(char* dst);

/**
 * @brief Reads the next line from yyin into a global buffer
 */
int getNextLine(void);

char* collapse_escape_sequences(char* string);

#endif // LEX_UTIL_H_
