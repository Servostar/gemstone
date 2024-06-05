//
// Created by servostar on 6/5/24.
//

#include <sys/log.h>
#include <sys/col.h>
#include <cfg/opt.h>
#include <mem/cache.h>

int main(int argc, char* argv[]) {
    mem_init();
    parse_options(argc, argv);
    log_init();
    set_log_level(LOG_LEVEL_DEBUG);
    col_init();

    for (int i = 0; i < 3; i++) {
        void* data = mem_alloc(MemoryNamespaceAst, 457);
        mem_realloc(MemoryNamespaceAst, data, 200);
        mem_free(data);
    }

    mem_purge_namespace(MemoryNamespaceOpt);

    print_memory_statistics();

    return 0;
}
