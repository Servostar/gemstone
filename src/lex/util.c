
#include <lex/util.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <mem/cache.h>

// implementation based on:
// https://github.com/sunxfancy/flex-bison-examples/blob/master/error-handling/ccalc.c

char* buffer = NULL;

static int eof = 0;
static int nRow = 0;
static int nBuffer = 0;
static int lBuffer = 0;
static int nTokenStart = 0;
static int nTokenLength = 0;
static int nTokenNextStart = 0;

void lex_init(void) {
  buffer = mem_alloc(MemoryNamespaceStatic, MAX_READ_BUFFER_SIZE);
}

void lex_reset(void) {
    eof = 0;
    nRow = 0;
    nBuffer = 0;
    lBuffer = 0;
    nTokenStart = 0;
    nTokenLength = 0;
    nTokenNextStart = 0;
}

void beginToken(char *t) {
  nTokenStart = nTokenNextStart;
  nTokenLength = (int) strlen(t);
  nTokenNextStart = nBuffer + 1;

  yylloc.first_line = nRow;
  yylloc.first_column = nTokenStart;
  yylloc.last_line = nRow;
  yylloc.last_column = nTokenStart + nTokenLength - 1;
}

int nextChar(char *dst) {
  int frc;

  if (eof)
    return 0;

  while (nBuffer >= lBuffer) {
    frc = getNextLine();
    if (frc != 0) {
      return 0;
    }
  }

  dst[0] = buffer[nBuffer];
  nBuffer += 1;

  return dst[0] != 0;
}

int getNextLine(void) {
  char *p;

  nBuffer = 0;
  nTokenStart = -1;
  nTokenNextStart = 1;
  eof = 0;

  p = fgets(buffer, MAX_READ_BUFFER_SIZE, yyin);
  if (p == NULL) {
    if (ferror(yyin)) {
      return -1;
    }
    eof = 1;
    return 1;
  }

  nRow += 1;
  lBuffer = (int) strlen(buffer);

  return 0;
}
