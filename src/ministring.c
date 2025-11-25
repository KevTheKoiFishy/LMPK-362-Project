#include "ministring.h"

// Return length of match
uint16_t mini_strcmp(char * str1, char * str2) {
    uint16_t len = 0;
    while ( *str1 != '\0' && *str2 != '\0' && *(str1++) == *(str2++) )
        { ++len; }
    return len;
}


// strtoi for any base 10 and under
uint16_t mini_strtoi(char * str, uint8_t len, uint8_t base) {
    uint16_t mul = 1;
    uint16_t sum = 0;
    char     c;

    str += len;
    while (len--) {
        c = *(--str);
        sum += (c - 48) * mul;
        mul *= base;
    }

    return sum;
}