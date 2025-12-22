#ifndef HAMCLOCK_CONFIG_H
#define HAMCLOCK_CONFIG_H

// Configuration management
int config_init(void);
int config_get(const char *key, char *value, int max_len);
int config_set(const char *key, const char *value);
int config_get_int(const char *key, int *value);
int config_set_int(const char *key, int value);

#endif // HAMCLOCK_CONFIG_H
