
#include <glib.h>

int main(int argc, char* argv[]) {

    GHashTable* map = g_hash_table_new(g_str_hash, g_str_equal);

    for (int i = 0; i < argc; i++) {
        int* index = malloc(sizeof(int));

        *index = i;

        g_hash_table_insert(map, argv[i], &index);
    }

    for (int i = 0; i < argc; i++) {
        int* index = (int*) g_hash_table_lookup(map, argv[i]);

        g_hash_table_remove(map, argv[i]);

        free(index);
    }

    g_hash_table_destroy(map);

    return 0;
}
