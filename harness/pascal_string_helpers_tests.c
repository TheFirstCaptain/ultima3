#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char Str255[256];
typedef bool Boolean;

static void BlockMoveData(const void *src, void *dst, size_t len)
{
    memmove(dst, src, len);
}

static void BlockMove(const void *src, void *dst, size_t len)
{
    memmove(dst, src, len);
}

static unsigned char StringLocation(Str255 source, Str255 search)
{
    /* Legacy reference: Sources/UltimaText.c StringLocation. */
    unsigned char i = 1;
    while (i < (source[0] - search[0])) {
        unsigned char j = 0;
        while (j < search[0] && (source[j + i] == search[j + 1])) {
            j++;
        }
        if (j == search[0])
            return i;
        i++;
    }
    return 0;
}

static void SearchReplace(Str255 source, Str255 search, Str255 replace)
{
    /* Legacy reference: Sources/UltimaText.c SearchReplace. */
    if (1) {
        char i = 1;
        char diff = replace[0] - search[0];
        while (i < source[0] - search[0]) {
            Boolean match = true;
            char offset = 0;
            while (match && offset < search[0]) {
                match = (source[i + offset] == search[1 + offset]);
                offset++;
            }
            if (match) {
                if (diff != 0)
                    BlockMoveData(source + i + search[0], source + i + replace[0], source[0] - i + diff + 1);
                BlockMoveData(replace + 1, source + i, replace[0]);
                source[0] += diff;
            }
            ++i;
        }
    }
}

static bool IsNewline(unsigned char ch)
{
    /* Legacy reference: Sources/UltimaText.c IsNewline. */
    return (ch == 0xB5 || ch == 0x0A);
}

static void AddString(Str255 str1, Str255 str2)
{
    /* Legacy reference: Sources/UltimaText.c AddString. */
    BlockMove(str2 + 1, str1 + str1[0] + 1, str2[0]);
    str1[0] += str2[0];
}

#define ASSERT_EQ_INT(expected, actual) assert_eq_int((expected), (actual), __FILE__, __LINE__)
#define ASSERT_PSTR(expected, actual) assert_pstr((expected), (actual), __FILE__, __LINE__)

static void assert_eq_int(int expected, int actual, const char *file, int line)
{
    if (expected != actual) {
        fprintf(stderr, "%s:%d: expected %d, got %d\n", file, line, expected, actual);
        exit(1);
    }
}

static void assert_pstr(const char *expected, const Str255 actual, const char *file, int line)
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

static void set_pstr(Str255 dest, const char *src)
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
    Str255 source;
    Str255 search;

    set_pstr(source, "say friend now");
    set_pstr(search, "friend");

    ASSERT_EQ_INT(5, StringLocation(source, search));
}

static void test_string_location_misses_last_possible_match(void)
{
    Str255 source;
    Str255 search;

    set_pstr(source, "say friend");
    set_pstr(search, "friend");

    ASSERT_EQ_INT(0, StringLocation(source, search));
}

static void test_string_location_misses_equal_length_match(void)
{
    Str255 source;
    Str255 search;

    set_pstr(source, "friend");
    set_pstr(search, "friend");

    ASSERT_EQ_INT(0, StringLocation(source, search));
}

static void test_search_replace_same_length(void)
{
    Str255 source;
    Str255 search;
    Str255 replace;

    set_pstr(source, "gold gem gold");
    set_pstr(search, "gold");
    set_pstr(replace, "coin");

    SearchReplace(source, search, replace);

    ASSERT_PSTR("coin gem gold", source);
}

static void test_search_replace_grows_string(void)
{
    Str255 source;
    Str255 search;
    Str255 replace;

    set_pstr(source, "a x b x c");
    set_pstr(search, "x");
    set_pstr(replace, "gem");

    SearchReplace(source, search, replace);

    ASSERT_PSTR("a gem b gem c", source);
}

static void test_search_replace_shrinks_string(void)
{
    Str255 source;
    Str255 search;
    Str255 replace;

    set_pstr(source, "open door open door");
    set_pstr(search, "door");
    set_pstr(replace, "d");

    SearchReplace(source, search, replace);

    ASSERT_PSTR("open d open door", source);
}

static void test_search_replace_misses_last_possible_match(void)
{
    Str255 source;
    Str255 search;
    Str255 replace;

    set_pstr(source, "door open door");
    set_pstr(search, "door");
    set_pstr(replace, "gate");

    SearchReplace(source, search, replace);

    ASSERT_PSTR("gate open door", source);
}

static void test_is_newline(void)
{
    ASSERT_EQ_INT(1, IsNewline(0x0A));
    ASSERT_EQ_INT(1, IsNewline(0xB5));
    ASSERT_EQ_INT(0, IsNewline('\r'));
    ASSERT_EQ_INT(0, IsNewline(' '));
}

static void test_add_string_appends_bytes_and_length(void)
{
    Str255 first;
    Str255 second;

    set_pstr(first, "Ultima");
    set_pstr(second, " III");

    AddString(first, second);

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

    AddString(first, second);

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
