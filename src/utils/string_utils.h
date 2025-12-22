#ifndef HAMCLOCK_STRING_UTILS_H
#define HAMCLOCK_STRING_UTILS_H

// String utilities
char *string_trim(char *str);
int string_split(char *str, char delimiter, char **parts, int max_parts);
int string_replace(char *str, int size, const char *old, const char *new);
char *string_dup(const char *str);
int string_startswith(const char *str, const char *prefix);
int string_endswith(const char *str, const char *suffix);

#endif // HAMCLOCK_STRING_UTILS_H
