
#include <stdlib.h>
#include <string.h>
#include <sys/col.h>
#include <sys/log.h>

#ifdef __unix__
#include <unistd.h>
#elif defined(_WIN32) || defined(WIN32)
#include <Windows.h>
#endif

char *RED;
char *YELLOW;
char *MAGENTA;
char *CYAN;
char *GREEN;
char *RESET;
char *BOLD;
char *FAINT;

void col_init(void) {
  if (stdout_supports_ansi_esc()) {
    enable_ansi_colors();
  } else {
    disable_ansi_colors();
  }
}

void disable_ansi_colors() {
  DEBUG("disabling ANSI escape codes");

  RED = "";
  YELLOW = "";
  MAGENTA = "";
  CYAN = "";
  GREEN = "";
  RESET = "";
  BOLD = "";
  FAINT = "";
}

void enable_ansi_colors() {
  DEBUG("enabling ANSI escape codes");

  RED = "\x1b[31m";
  YELLOW = "\x1b[33m";
  MAGENTA = "\x1b[35m";
  CYAN = "\x1b[36m";
  GREEN = "\x1b[32m";
  RESET = "\x1b[0m";
  BOLD = "\x1b[1m";
  FAINT = "\x1b[2m";
}

int stdout_supports_ansi_esc() {

#ifdef __unix__
  // check if TTY
  if (isatty(STDOUT_FILENO)) {
    const char *colors = getenv("COLORTERM");
    // check if colors are set and allowed
    if (colors != NULL && (strcmp(colors, "truecolor") == 0 || strcmp(colors, "24bit") == 0)) {
      return ANSI_ENABLED;
    }
  }
#elif defined(_WIN32) || defined(WIN32)
  // see:
  // https://stackoverflow.com/questions/63913005/how-to-test-if-console-supports-ansi-color-codes
  DWORD mode;
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  if (!GetConsoleMode(hConsole, &mode)) {
    ERROR("failed to get console mode");
    return ANSI_ENABLED;
  }

  if ((mode & ENABLE_VIRTUAL_TERMINAL_INPUT) |
      (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
    return ANSI_ENABLED;
  }
#else
#warning "unsupported platform, ASNI escape codes disabled by default"
#endif

  return ASNI_DISABLED;
}
