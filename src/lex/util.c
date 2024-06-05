
#include <lex/util.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

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

static GArray* stringCache = NULL;

char* lex_cached_strdup(char* string) {
    char* dup = strdup(string);

    g_array_append_val(stringCache, dup);

    return dup;
}

void lex_purge_str_cache() {
    DEBUG("purging string cache...");

    const guint count = stringCache->len;

    for (guint i = 0; i < count; i++) {
        free(((char**) stringCache->data)[i]);
    }

    g_array_remove_range(stringCache, 0, count);
}

static void lex_deinit(void) {
    lex_purge_str_cache();
    g_array_free(stringCache, TRUE);
  free(buffer);
}

void lex_init(void) {
  buffer = malloc(MAX_READ_BUFFER_SIZE);
    stringCache = g_array_new(FALSE, FALSE, sizeof(char*));
  atexit(lex_deinit);
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
