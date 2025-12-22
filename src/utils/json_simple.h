#ifndef HAMCLOCK_JSON_SIMPLE_H
#define HAMCLOCK_JSON_SIMPLE_H

// Simple JSON value extraction (no external library needed)
// Handles basic JSON parsing for APIs

typedef struct {
    const char *data;
    int size;
} json_obj_t;

// Extract string value from JSON by key
// Returns 0 on success, -1 if key not found
int json_get_string(const char *json, const char *key, char *value, int max_len);

// Extract float value from JSON by key
int json_get_float(const char *json, const char *key, float *value);

// Extract int value from JSON by key
int json_get_int(const char *json, const char *key, int *value);

// Find array element by index (for array[n].key)
// Returns pointer to start of array element, or NULL
const char *json_get_array_element(const char *json, const char *array_name, int index);

#endif // HAMCLOCK_JSON_SIMPLE_H
