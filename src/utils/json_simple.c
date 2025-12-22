#include "json_simple.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Find a key in JSON and return pointer to its value
static const char *json_find_key(const char *json, const char *key) {
    if (!json || !key) return NULL;

    int key_len = strlen(key);
    const char *ptr = json;

    while ((ptr = strchr(ptr, '"')) != NULL) {
        ptr++;  // Skip opening quote

        // Check if this is our key
        if (strncmp(ptr, key, key_len) == 0 && ptr[key_len] == '"') {
            // Found the key, now find the colon
            ptr += key_len + 1;
            while (*ptr && *ptr != ':') ptr++;
            if (*ptr == ':') {
                ptr++;
                // Skip whitespace
                while (*ptr && isspace(*ptr)) ptr++;
                return ptr;
            }
        }

        // Skip to next quote
        while (*ptr && *ptr != '"') {
            if (*ptr == '\\') ptr++;  // Skip escaped chars
            if (*ptr) ptr++;
        }
    }

    return NULL;
}

int json_get_string(const char *json, const char *key, char *value, int max_len) {
    if (!json || !key || !value || max_len <= 0) return -1;

    const char *ptr = json_find_key(json, key);
    if (!ptr || *ptr != '"') return -1;

    ptr++;  // Skip opening quote
    int i = 0;
    while (i < max_len - 1 && *ptr && *ptr != '"') {
        if (*ptr == '\\') {
            ptr++;
            if (*ptr == 'n') value[i++] = '\n';
            else if (*ptr == 't') value[i++] = '\t';
            else if (*ptr == 'r') value[i++] = '\r';
            else value[i++] = *ptr;
        } else {
            value[i++] = *ptr;
        }
        ptr++;
    }
    value[i] = 0;
    return 0;
}

int json_get_float(const char *json, const char *key, float *value) {
    if (!json || !key || !value) return -1;

    const char *ptr = json_find_key(json, key);
    if (!ptr) return -1;

    // Skip null values
    if (strncmp(ptr, "null", 4) == 0) {
        *value = 0.0f;
        return 0;
    }

    // Try to parse as float
    *value = strtof(ptr, NULL);
    return 0;
}

int json_get_int(const char *json, const char *key, int *value) {
    if (!json || !key || !value) return -1;

    const char *ptr = json_find_key(json, key);
    if (!ptr) return -1;

    // Skip null values
    if (strncmp(ptr, "null", 4) == 0) {
        *value = 0;
        return 0;
    }

    // Try to parse as int
    *value = (int)strtol(ptr, NULL, 10);
    return 0;
}

const char *json_get_array_element(const char *json, const char *array_name, int index) {
    if (!json || !array_name || index < 0) return NULL;

    // Find the array start
    int name_len = strlen(array_name);
    const char *ptr = json;

    while ((ptr = strchr(ptr, '"')) != NULL) {
        ptr++;
        if (strncmp(ptr, array_name, name_len) == 0 && ptr[name_len] == '"') {
            // Found array name, find opening bracket
            ptr += name_len + 1;
            while (*ptr && *ptr != '[') ptr++;
            if (*ptr != '[') return NULL;
            ptr++;

            // Skip 'index' array elements
            int current_index = 0;
            while (*ptr && current_index < index) {
                if (*ptr == '{') {
                    // Skip to closing brace
                    int depth = 1;
                    ptr++;
                    while (*ptr && depth > 0) {
                        if (*ptr == '{') depth++;
                        else if (*ptr == '}') depth--;
                        ptr++;
                    }
                    current_index++;
                    // Skip comma and whitespace
                    while (*ptr && (*ptr == ',' || isspace(*ptr))) ptr++;
                } else {
                    ptr++;
                }
            }

            // Skip whitespace and should be at opening brace
            while (*ptr && isspace(*ptr)) ptr++;
            if (*ptr == '{') return ptr;
        }

        // Try next quote
        while (*ptr && *ptr != '"') {
            if (*ptr == '\\') ptr++;
            if (*ptr) ptr++;
        }
    }

    return NULL;
}
