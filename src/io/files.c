//
// Created by servostar on 5/30/24.
//

#include <io/files.h>
#include <sys/log.h>
#include <assert.h>
#include <sys/col.h>

ModuleFile *push_file(ModuleFileStack *stack, const char *path) {
    assert(stack != NULL);

    // lazy init of heap stack
    if (stack->files == NULL) {
        stack->files = g_array_new(FALSE, FALSE, sizeof(ModuleFile));
    }

    ModuleFile new_file = {
            .path = path,
            .handle = NULL
    };

    g_array_append_val(stack->files, new_file);

    return ((ModuleFile *) stack->files->data) + stack->files->len - 1;
}

void delete_files(ModuleFileStack *stack) {
    for (size_t i = 0; i < stack->files->len; i++) {
        ModuleFile *file = ((ModuleFile *) stack->files->data) + i;

        if (file->handle != NULL) {
            DEBUG("closing file: %s", file->path);
            fclose(file->handle);
        }

    }

    g_array_free(stack->files, TRUE);
    DEBUG("deleted module file stack");
}

#define SEEK_BUF_BYTES 256

static inline unsigned long int min(unsigned long int a, unsigned long int b) {
    return a > b ? b : a;
}

// behaves like fgets except that it has defined behavior when n == 1
static void custom_fgets(char *buffer, size_t n, FILE *stream) {
    if (n == 1) {
        buffer[0] = (char) fgetc(stream);
        buffer[1] = 0;
    } else {
        fgets(buffer, (int) n, stream);
    }
}

void print_diagnostic(ModuleFile *file, TokenLocation *location, Message kind, const char *message) {
    assert(file->handle != NULL);
    assert(location != NULL);
    assert(message != NULL);

    // reset to start
    rewind(file->handle);

    char *buffer = alloca(SEEK_BUF_BYTES);
    unsigned long int line_count = 1;

    // seek to first line
    while (line_count < location->line_start && fgets(buffer, SEEK_BUF_BYTES, file->handle) != NULL) {
        line_count += strchr(buffer, '\n') != NULL;
    }

    const char *accent_color = RESET;
    const char *kind_text = "unknown";
    switch (kind) {
        case Info:
            kind_text = "info";
            accent_color = CYAN;
            file->statistics.info_count++;
            break;
        case Warning:
            kind_text = "warning";
            accent_color = YELLOW;
            file->statistics.warning_count++;
            break;
        case Error:
            kind_text = "error";
            accent_color = RED;
            file->statistics.error_count++;
            break;
    }

    char absolute_path[PATH_MAX];
    realpath(file->path, absolute_path);

    printf("%s%s:%ld:%s %s%s:%s %s\n", BOLD, absolute_path, location->line_start, RESET, accent_color, kind_text, RESET,
           message);

    size_t lines = location->line_end - location->line_start + 1;

    for (size_t l = 0; l < lines; l++) {
        printf(" %4ld | ", location->line_start + l);

        size_t limit;
        size_t chars = 0;

        // print line before token group start
        limit = min(location->col_start, SEEK_BUF_BYTES);
        while (limit > 1) {
            custom_fgets(buffer, (int) limit, file->handle);
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
            custom_fgets(buffer, (int) limit, file->handle);
            chars += printf("%s", buffer);
            limit = min(location->col_end - location->col_start + 1 - chars, SEEK_BUF_BYTES);

            if (strchr(buffer, '\n') != NULL) {
                goto cont;
            }
        }

        printf("%s", RESET);

        // print rest of the line
        do {
            custom_fgets(buffer, SEEK_BUF_BYTES, file->handle);
            printf("%s", buffer);
        } while (strchr(buffer, '\n') == NULL);

        cont:
        printf("%s", RESET);
    }

    printf("      | ");
    for (size_t i = 1; i < location->col_start; i++) {
        printf(" ");
    }

    printf("%s", accent_color);
    printf("^");
    for (size_t i = 0; i < location->col_end - location->col_start; i++) {
        printf("~");
    }

    printf("%s\n\n", RESET);
}

TokenLocation new_location(unsigned long int line_start, unsigned long int col_start, unsigned long int line_end,
                           unsigned long int col_end) {
    TokenLocation location;

    location.line_start = line_start;
    location.line_end = line_end;
    location.col_start = col_start;
    location.col_end = col_end;

    return location;
}

TokenLocation empty_location(void) {
    TokenLocation location;

    location.line_start = 0;
    location.line_end = 0;
    location.col_start = 0;
    location.col_end = 0;

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
        ModuleFile *file = (ModuleFile *) file_stack->files->data;

        stats.info_count += file->statistics.warning_count;
        stats.warning_count += file->statistics.warning_count;
        stats.error_count += file->statistics.error_count;
    }

    printf("%d files generated ", file_stack->files->len);

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