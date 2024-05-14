#include <ast/ast.h>
#include <stdlib.h>
#include <sys/log.h>
#include <yacc/parser.tab.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG

extern FILE *yyin;

// Global array to store options
char options[5][10];
int num_options = 0;

/**
 * @brief Log a debug message to inform about beginning exit procedures
 *
 */
void notify_exit(void) { DEBUG("Exiting gemstone..."); }


/**
 * @brief add option to global option array
 *
 */

void add_option(const char* option) {
    if (num_options < 5 ) {
        strcpy(options[num_options], option);
        num_options++;
    } else {
        PANIC("Too Many Options given");
    }
}

/**
 * @brief Check if Option is set
 *
 */

size_t check_option(const char* name) {
    for (int i = 0; i < num_options; i++) {
        if (strcmp(options[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Closes File after compiling.
 *
 */

void close_file(void) {
  if (NULL != yyin) {
    fclose(yyin);
  }
}

/**
 * @brief Run compiler setup here
 *
 */
void setup(void) {
  // setup preample
  log_init();
  DEBUG("starting gemstone...");

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
  atexit(&notify_exit);
#endif

  // actual setup
  AST_init();

  DEBUG("finished starting up gemstone...");
}

int main(int argc, char *argv[]) {

   // Iteration through arguments
    for (int i = 1; i < argc; i++) {
        // Check if the argument starts with "--"
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            // Extract option name
            char option[10];
            strcpy(option, argv[i] + 2);

            // Add option to the global array
            add_option(option);
        }
    }
  setup();
  atexit(close_file);

  // Check for file input as argument
  if (2 != argc) {
    INFO("Usage: %s <filename>\n", argv[0]);
    PANIC("No File could be found");
  }

  // filename as first argument
  char *filename = argv[1];

  FILE *file = fopen(filename, "r");

  if (NULL == file) {
    PANIC("File couldn't be opened!");
  }
  yyin = file;

  yyparse();

  return 0;
}
