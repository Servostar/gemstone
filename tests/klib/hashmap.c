
#include <klib/khash.h>

struct Book {
    const char* author;
    size_t pages;
};

KHASH_MAP_INIT_STR(books, struct Book)

void put(const char* key, const struct Book book, khash_t(books)* books) {
    khint_t idx;
    int ret;
    
    idx = kh_put(books, books, key, &ret);

    if (!ret)
        kh_del(books, books, idx);

    kh_value(books, idx) = book;
}

int main(void) {

    khash_t(books) *map = kh_init(books);

    struct Book book;
    book.author = "Bob";
    book.pages = 45;

    put("Pharao of Egypt", book, map);

    kh_destroy(books, map);

    return 0;
}
