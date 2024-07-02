//
// Created by servostar on 5/30/24.
//

#include <io/files.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/log.h>
#include <assert.h>
#include <sys/col.h>
#include <mem/cache.h>

#ifdef __unix__

#include <sys/stat.h>

#define MAX_PATH_BYTES PATH_MAX

#define min(a, b) ((a) > (b) ? (b) : (a))

#elif defined(_WIN32) || defined(WIN32)

#include <Windows.h>
// for _fullpath
#include <stdlib.h>
// for _mkdir
#include <direct.h>

#define MAX_PATH_BYTES _MAX_PATH

#endif

ModuleFileStack new_file_stack() {
    ModuleFileStack stack;
    stack.files = NULL;

    return stack;
}

ModuleFile *push_file(ModuleFileStack *stack, const char *path) {
    assert(stack != NULL);

    // lazy init of heap stack
    if (stack->files == NULL) {
        stack->files = g_array_new(FALSE, FALSE, sizeof(ModuleFile*));
    }

    ModuleFile* new_file = mem_alloc(MemoryNamespaceStatic, sizeof(ModuleFile));
    new_file->handle = NULL;
    new_file->path = path;
    new_file->statistics.warning_count = 0;
    new_file->statistics.error_count = 0;
    new_file->statistics.info_count = 0;

    g_array_append_val(stack->files, new_file);

    return new_file;
}

void delete_files(ModuleFileStack *stack) {
    for (size_t i = 0; i < stack->files->len; i++) {
        const ModuleFile *file = g_array_index(stack->files, ModuleFile*, i);

        if (file->handle != NULL) {
            DEBUG("closing file: %s", file->path);
            fclose(file->handle);
        }

    }

    g_array_free(stack->files, TRUE);
    DEBUG("deleted module file stack");
}

// Number of bytes to read at once whilest
// seeking the current line in print_diagnostic()
#define SEEK_BUF_BYTES 256

// behaves like fgets except that it has defined behavior when n == 1
static void custom_fgets(char *buffer, size_t n, FILE *stream) {
    if (feof(stream)) {
        buffer[0] = '\n';
        buffer[1] = 0;
        return;
    }
    if (n == 1) {
        buffer[0] = (char) fgetc(stream);
        buffer[1] = 0;
    } else {
        fgets(buffer, (int) n, stream);
    }
}

void print_diagnostic(TokenLocation *location, Message kind, const char *message, ...) {
    assert(location->file != NULL);
    assert(location->file->handle != NULL);
    assert(location != NULL);
    assert(message != NULL);

    // reset to start
    rewind(location->file->handle);

    char *buffer = alloca(SEEK_BUF_BYTES);
    unsigned long int line_count = 1;

    // seek to first line
    while (line_count < location->line_start && fgets(buffer, SEEK_BUF_BYTES, location->file->handle) != NULL) {
        line_count += strchr(buffer, '\n') != NULL;
    }

    const char *accent_color = RESET;
    const char *kind_text = "unknown";
    switch (kind) {
        case Info:
            kind_text = "info";
            accent_color = CYAN;
            location->file->statistics.info_count++;
            break;
        case Warning:
            kind_text = "warning";
            accent_color = YELLOW;
            location->file->statistics.warning_count++;
            break;
        case Error:
            kind_text = "error";
            accent_color = RED;
            location->file->statistics.error_count++;
            break;
    }

    const char *absolute_path = get_absolute_path(location->file->path);

    printf("%s%s:%ld:%s %s%s:%s ", BOLD, absolute_path, location->line_start, RESET, accent_color, kind_text, RESET);

    va_list args;
    va_start(args, message);

    vprintf(message, args);

    va_end(args);

    printf("\n");

    mem_free((void *) absolute_path);

    const unsigned long int lines = location->line_end - location->line_start + 1;

    for (unsigned long int l = 0; l < lines; l++) {
        printf(" %4ld | ", location->line_start + l);

        unsigned long int chars = 0;

        // print line before token group start
        unsigned long int limit = min(location->col_start, SEEK_BUF_BYTES);
        while (limit > 1) {
            custom_fgets(buffer, (int) limit, location->file->handle);
            chars += printf("%s", buffer);
            limit = min(location->col_start - chars, SEEK_BUF_BYTES);

            if (strchr(buffer, '\n') != NULL) {
                goto cont;
            }
        }

        printf("%s", accent_color);

        chars = 0;
        limit = min(location->col_end - location->col_start + 1, SEEK_BUF_BYTES);
        while (limit > 0) {
            custom_fgets(buffer, (int) limit, location->file->handle);
            chars += printf("%s", buffer);
            limit = min(location->col_end - location->col_start + 1 - chars, SEEK_BUF_BYTES);

            if (strchr(buffer, '\n') != NULL) {
                goto cont;
            }
        }

        printf("%s", RESET);

        // print rest of the line
        do {
            custom_fgets(buffer, SEEK_BUF_BYTES, location->file->handle);
            printf("%s", buffer);
        } while (strchr(buffer, '\n') == NULL);

        cont:
        printf("%s", RESET);
    }

    printf("      | ");
    for (unsigned long int i = 1; i < location->col_start; i++) {
        printf(" ");
    }

    printf("%s", accent_color);
    printf("^");
    for (unsigned long int i = 0; i < location->col_end - location->col_start; i++) {
        printf("~");
    }

    printf("%s\n\n", RESET);
}

TokenLocation new_location(unsigned long int line_start, unsigned long int col_start, unsigned long int line_end,
                           unsigned long int col_end, ModuleFile* file) {
    TokenLocation location;

    location.line_start = line_start;
    location.line_end = line_end;
    location.col_start = col_start;
    location.col_end = col_end;
    location.file = file;

    return location;
}

TokenLocation empty_location(ModuleFile* file) {
    TokenLocation location;

    location.line_start = 0;
    location.line_end = 0;
    location.col_start = 0;
    location.col_end = 0;
    location.file = file;

    return location;
}

void print_file_statistics(ModuleFile *file) {
    if (file->statistics.info_count + file->statistics.warning_count + file->statistics.error_count < 1) {
        return;
    }

    printf("File %s generated ", file->path);

    if (file->statistics.info_count > 0) {
        printf("%ld notice(s) ", file->statistics.info_count);
    }

    if (file->statistics.warning_count > 0) {
        printf("%ld warning(s) ", file->statistics.warning_count);
    }

    if (file->statistics.error_count > 0) {
        printf("%ld error(s) ", file->statistics.error_count);
    }

    printf("\n\n");
}

void print_unit_statistics(ModuleFileStack *file_stack) {
    FileDiagnosticStatistics stats;
    stats.info_count = 0;
    stats.warning_count = 0;
    stats.error_count = 0;

    for (size_t i = 0; i < file_stack->files->len; i++) {
        ModuleFile* file = g_array_index(file_stack->files, ModuleFile*, i);

        stats.info_count += file->statistics.warning_count;
        stats.warning_count += file->statistics.warning_count;
        stats.error_count += file->statistics.error_count;
    }

    if (stats.info_count + stats.warning_count + stats.error_count < 1) {
        return;
    }

    printf("%d file(s) generated ", file_stack->files->len);

    if (stats.info_count > 0) {
        printf("%ld notice(s) ", stats.info_count);
    }

    if (stats.warning_count > 0) {
        printf("%ld warning(s) ", stats.warning_count);
    }

    if (stats.error_count > 0) {
        printf("%ld error(s) ", stats.error_count);
    }

    printf("\n\n");
}

void print_message(Message kind, const char *fmt, ...) {
    const char *accent_color = RESET;
    const char *kind_text = "unknown";
    switch (kind) {
        case Info:
            kind_text = "info";
            accent_color = CYAN;
            break;
        case Warning:
            kind_text = "warning";
            accent_color = YELLOW;
            break;
        case Error:
            kind_text = "error";
            accent_color = RED;
            break;
    }

    va_list args;
    va_start(args, fmt);

    printf("%s%s:%s ", accent_color, kind_text, RESET);
    vprintf(fmt, args);
    printf("\n");

    va_end(args);
}

int create_directory(const char *path) {
    assert(path != NULL);

    DEBUG("creating directory: %s", path);

    return g_mkdir_with_parents(path, 0755);
}

const char *get_last_error() {
    return mem_strdup(MemoryNamespaceIo, strerror(errno));
}

const char *get_absolute_path(const char *path) {
    assert(path != NULL);

    DEBUG("resolving absolute path of: %s", path);

    char* cwd = g_get_current_dir();
    char* canoical = g_canonicalize_filename(path, cwd);
    g_free(cwd);

    return canoical;
}
