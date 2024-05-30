//
// Created by servostar on 5/30/24.
//

#ifndef GEMSTONE_FILES_H
#define GEMSTONE_FILES_H

#include <stdio.h>
#include <glib.h>

typedef struct FileDiagnosticStatistics_t {
    size_t error_count;
    size_t warning_count;
    size_t info_count;
} FileDiagnosticStatistics;

typedef struct ModuleFile_t {
    const char *path;
    FILE *handle;
    FileDiagnosticStatistics statistics;
} ModuleFile;

typedef struct ModuleFileStack_t {
    GArray *files;
} ModuleFileStack;

typedef enum Message_t {
    Info,
    Warning,
    Error
} Message;

typedef struct TokenLocation_t {
    unsigned long int line_start;
    unsigned long int col_start;
    unsigned long int line_end;
    unsigned long int col_end;
} TokenLocation;

/**
 * @brief Add a new file to the file stack.
 * @attention The file handle returned will be invalid
 * @param stack
 * @param path
 * @return A new file module
 */
[[gnu::nonnull(1), gnu::nonnull(2)]]
ModuleFile *push_file(ModuleFileStack *stack, const char *path);

/**
 * @brief Delete all files in the stack and the stack itself
 * @param stack
 */
[[gnu::nonnull(1)]]
void delete_files(ModuleFileStack *stack);

/**
 * Create a new token location
 * @param line_start
 * @param col_start
 * @param line_end
 * @param col_end
 * @return
 */
TokenLocation new_location(unsigned long int line_start, unsigned long int col_start, unsigned long int line_end,
                           unsigned long int col_end);

TokenLocation empty_location(void);

[[gnu::nonnull(1), gnu::nonnull(2)]]
void print_diagnostic(ModuleFile *file, TokenLocation *location, Message kind, const char *message);

[[gnu::nonnull(1)]]
void print_file_statistics(ModuleFile *file);

[[gnu::nonnull(1)]]
void print_unit_statistics(ModuleFileStack *file_stack);

#endif //GEMSTONE_FILES_H
