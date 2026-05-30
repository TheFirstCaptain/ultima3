#ifndef U3_PASCAL_STRING_H
#define U3_PASCAL_STRING_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u3_pascal_string[256];

uint8_t u3_string_location(u3_pascal_string source, u3_pascal_string search);
void u3_search_replace(u3_pascal_string source, u3_pascal_string search, u3_pascal_string replace);
bool u3_is_newline(uint8_t ch);
void u3_add_string(u3_pascal_string str1, u3_pascal_string str2);

#endif
