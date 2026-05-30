#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u3_pascal_string.h"

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_PSTR(expected, actual) assert_pstr((expected), (actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void assert_pstr(const char *expected, const u3_pascal_string actual, const char *file, int line)
{
    size_t expected_len = strlen(expected);
    size_t i;

    if (actual[0] != expected_len) {
        fprintf(stderr, "%s:%d: expected length %zu, got %u; actual \"", file, line, expected_len, actual[0]);
        for (i = 1; i <= actual[0]; i++) {
            fputc(actual[i], stderr);
        }
        fputs("\"\n", stderr);
        exit(1);
    }

    for (i = 0; i < expected_len; i++) {
        if (actual[i + 1] != (unsigned char)expected[i]) {
            fprintf(stderr, "%s:%d: byte %zu expected 0x%02x, got 0x%02x\n",
                    file, line, i + 1, (unsigned char)expected[i], actual[i + 1]);
            exit(1);
        }
    }
}

static void set_pstr(u3_pascal_string dest, const char *src)
{
    size_t len = strlen(src);

    if (len > 255) {
        fprintf(stderr, "fixture too long\n");
        exit(1);
    }

    memset(dest, 0, 256);
    dest[0] = (unsigned char)len;
    memcpy(dest + 1, src, len);
}

static void test_string_location_finds_non_terminal_match(void)
{
    u3_pascal_string source;
    u3_pascal_string search;

    set_pstr(source, "say friend now");
    set_pstr(search, "friend");

    ASSERT_EQ_INT(5, u3_string_location(source, search));
}

static void test_string_location_misses_last_possible_match(void)
{
    u3_pascal_string source;
    u3_pascal_string search;

    set_pstr(source, "say friend");
    set_pstr(search, "friend");

    ASSERT_EQ_INT(0, u3_string_location(source, search));
}

static void test_string_location_misses_equal_length_match(void)
{
    u3_pascal_string source;
    u3_pascal_string search;

    set_pstr(source, "friend");
    set_pstr(search, "friend");

    ASSERT_EQ_INT(0, u3_string_location(source, search));
}

static void test_search_replace_same_length(void)
{
    u3_pascal_string source;
    u3_pascal_string search;
    u3_pascal_string replace;

    set_pstr(source, "gold gem gold");
    set_pstr(search, "gold");
    set_pstr(replace, "coin");

    u3_search_replace(source, search, replace);

    ASSERT_PSTR("coin gem gold", source);
}

static void test_search_replace_grows_string(void)
{
    u3_pascal_string source;
    u3_pascal_string search;
    u3_pascal_string replace;

    set_pstr(source, "a x b x c");
    set_pstr(search, "x");
    set_pstr(replace, "gem");

    u3_search_replace(source, search, replace);

    ASSERT_PSTR("a gem b gem c", source);
}

static void test_search_replace_shrinks_string(void)
{
    u3_pascal_string source;
    u3_pascal_string search;
    u3_pascal_string replace;

    set_pstr(source, "open door open door");
    set_pstr(search, "door");
    set_pstr(replace, "d");

    u3_search_replace(source, search, replace);

    ASSERT_PSTR("open d open door", source);
}

static void test_search_replace_misses_last_possible_match(void)
{
    u3_pascal_string source;
    u3_pascal_string search;
    u3_pascal_string replace;

    set_pstr(source, "door open door");
    set_pstr(search, "door");
    set_pstr(replace, "gate");

    u3_search_replace(source, search, replace);

    ASSERT_PSTR("gate open door", source);
}

static void test_is_newline(void)
{
    ASSERT_EQ_INT(1, u3_is_newline(0x0A));
    ASSERT_EQ_INT(1, u3_is_newline(0xB5));
    ASSERT_EQ_INT(0, u3_is_newline('\r'));
    ASSERT_EQ_INT(0, u3_is_newline(' '));
}

static void test_add_string_appends_bytes_and_length(void)
{
    u3_pascal_string first;
    u3_pascal_string second;

    set_pstr(first, "Ultima");
    set_pstr(second, " III");

    u3_add_string(first, second);

    ASSERT_PSTR("Ultima III", first);
    ASSERT_PSTR(" III", second);
}

static void test_add_string_allows_length_byte_wrap(void)
{
    unsigned char first[270];
    unsigned char second[16];
    int i;

    memset(first, 'A', sizeof(first));
    memset(second, 'B', sizeof(second));
    first[0] = 250;
    second[0] = 10;

    u3_add_string(first, second);

    ASSERT_EQ_INT(4, first[0]);
    for (i = 251; i <= 255; i++) {
        ASSERT_EQ_INT('B', first[i]);
    }
}

int main(void)
{
    test_string_location_finds_non_terminal_match();
    test_string_location_misses_last_possible_match();
    test_string_location_misses_equal_length_match();
    test_search_replace_same_length();
    test_search_replace_grows_string();
    test_search_replace_shrinks_string();
    test_search_replace_misses_last_possible_match();
    test_is_newline();
    test_add_string_appends_bytes_and_length();
    test_add_string_allows_length_byte_wrap();

    puts("Pascal string helper characterization passed");
    return 0;
}
