#include "string_utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *string_trim(char *str) {
    if (!str) return str;

    // Trim leading whitespace
    while (*str && isspace((unsigned char)*str)) str++;

    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return str;
}

int string_split(char *str, char delimiter, char **parts, int max_parts) {
    if (!str || !parts || max_parts <= 0) return 0;

    int count = 0;
    char *current = str;
    char *next;

    while (current && count < max_parts) {
        next = strchr(current, delimiter);
        if (next) {
            *next = '\0';
            parts[count++] = current;
            current = next + 1;
        } else {
            parts[count++] = current;
            break;
        }
    }

    return count;
}

int string_replace(char *str, int size, const char *old, const char *new) {
    if (!str || !old || !new) return -1;

    char *pos = strstr(str, old);
    if (!pos) return 0;  // Not found

    int old_len = strlen(old);
    int new_len = strlen(new);
    int diff = new_len - old_len;

    if (strlen(str) + diff >= size) return -1;  // Not enough space

    if (diff != 0) {
        memmove(pos + new_len, pos + old_len, strlen(pos + old_len) + 1);
    }

    memcpy(pos, new, new_len);
    return 1;  // Replaced
}

char *string_dup(const char *str) {
    if (!str) return NULL;
    char *copy = malloc(strlen(str) + 1);
    if (copy) strcpy(copy, str);
    return copy;
}

int string_startswith(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int string_endswith(const char *str, const char *suffix) {
    if (!str || !suffix) return 0;
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);
    if (suffix_len > str_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}
