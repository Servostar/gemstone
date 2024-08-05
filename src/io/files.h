//
// Created by servostar on 5/30/24.
//

#ifndef GEMSTONE_FILES_H
#define GEMSTONE_FILES_H

#include <glib.h>
#include <stdio.h>

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

typedef struct FileDiagnosticStatistics_t {
    unsigned long int error_count;
    unsigned long int warning_count;
    unsigned long int info_count;
} FileDiagnosticStatistics;

typedef struct ModuleFile_t {
    const char* path;
    FILE* handle;
    FileDiagnosticStatistics statistics;
} ModuleFile;

typedef struct ModuleFileStack_t {
    GArray* files;
} ModuleFileStack;

typedef enum Message_t { Info, Warning, Error } Message;

typedef struct TokenLocation_t {
    unsigned long int line_start;
    unsigned long int col_start;
    unsigned long int line_end;
    unsigned long int col_end;
    ModuleFile* file;
} TokenLocation;

/**
 * @brief Create a new, empty file stack.
 * @return
 */
ModuleFileStack new_file_stack();

/**
 * @brief Add a new file to the file stack.
 * @attention The file handle returned will be invalid
 * @param stack
 * @param path
 * @return A new file module
 */
[[gnu::nonnull(1), gnu::nonnull(2)]]
ModuleFile* push_file(ModuleFileStack* stack, const char* path);

/**
 * @brief Delete all files in the stack and the stack itself
 * @param stack
 */
[[gnu::nonnull(1)]]
void delete_files(ModuleFileStack* stack);

/**
 * Create a new token location
 * @param line_start
 * @param col_start
 * @param line_end
 * @param col_end
 * @return
 */
TokenLocation new_location(unsigned long int line_start,
                           unsigned long int col_start,
                           unsigned long int line_end,
                           unsigned long int col_end, ModuleFile* file);

/**
 * @brief Create a new empty location with all of its contents set to zero
 * @return
 */
TokenLocation empty_location(ModuleFile* file);

/**
 * @brief Prints some diagnostic message to stdout.
 *        This also print the token group and the attached source as context.
 * @param location
 * @param kind
 * @param message
 */
[[gnu::nonnull(1)]]
void print_diagnostic(TokenLocation* location, Message kind,
                      const char* message, ...);

[[gnu::nonnull(2)]]
/**
 * @brief Print a general message to stdout. Provides no source context like print_diagnostic()
 * @param kind
 * @param fmt
 * @param ...
 */
void print_message(Message kind, const char* fmt, ...);

/**
 * @brief Print statistics about a specific file.
 *        Will print the amount of infos, warning and errors emitted during
 * compilation.
 * @param file
 */
[[gnu::nonnull(1)]]
void print_file_statistics(ModuleFile* file);

/**
 * @brief Print statistics of all files in the module stack.
 * @param file_stack
 */
[[gnu::nonnull(1)]]
void print_unit_statistics(ModuleFileStack* file_stack);

/**
 * @brief Create a new directory. Will return EEXISTS in case the directory
 * already exists.
 * @param path
 * @return 0 if successful, anything else otherwise
 */
[[gnu::nonnull(1)]]
int create_directory(const char* path);

/**
 * @brief Get a string describing the last error set by errno.
 * @return a string that must be freed
 */
[[nodiscard("pointer must be freed")]]
const char* get_last_error();

/**
 * @brief Resolve the absolute path from a given relative path.
 * @param path
 * @return
 */
[[gnu::nonnull(1)]] [[nodiscard("pointer must be freed")]]
const char* get_absolute_path(const char* path);

#endif // GEMSTONE_FILES_H
