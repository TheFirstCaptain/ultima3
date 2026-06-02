#include "u3_pascal_string.h"

#include <string.h>

static void u3_block_move_data(const void *src, void *dst, size_t len)
{
    memmove(dst, src, len);
}

static void u3_block_move(const void *src, void *dst, size_t len)
{
    memmove(dst, src, len);
}

uint8_t u3_string_location(u3_pascal_string source, u3_pascal_string search)
{
    uint8_t i = 1;

    while (i < (source[0] - search[0])) {
        uint8_t j = 0;

        while (j < search[0] && (source[j + i] == search[j + 1])) {
            j++;
        }
        if (j == search[0])
            return i;
        i++;
    }
    return 0;
}

void u3_search_replace(u3_pascal_string source, u3_pascal_string search, u3_pascal_string replace)
{
    if (1) {
        int i = 1;
        int diff = (int)replace[0] - (int)search[0];

        while (i < source[0] - search[0]) {
            bool match = true;
            int offset = 0;

            while (match && offset < search[0]) {
                match = (source[i + offset] == search[1 + offset]);
                offset++;
            }
            if (match) {
                if (diff != 0)
                    u3_block_move_data(source + i + search[0], source + i + replace[0], source[0] - i + diff + 1);
                u3_block_move_data(replace + 1, source + i, replace[0]);
                source[0] += diff;
            }
            ++i;
        }
    }
}

bool u3_is_newline(uint8_t ch)
{
    return (ch == 0xB5 || ch == 0x0A);
}

void u3_add_string(u3_pascal_string str1, u3_pascal_string str2)
{
    u3_block_move(str2 + 1, str1 + str1[0] + 1, str2[0]);
    str1[0] += str2[0];
}
