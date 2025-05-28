#include "keyValStore.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    int in_use;
} KeyValue;

static KeyValue store[MAX_ENTRIES];

int initStore() {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        store[i].in_use = 0;
    }
    return 0;
}

int put(const char *key, const char *value) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (store[i].in_use && strcmp(store[i].key, key) == 0) {
            strncpy(store[i].value, value, MAX_VALUE_LEN - 1);
            store[i].value[MAX_VALUE_LEN - 1] = '\0';
            return 0;
        }
    }

    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!store[i].in_use) {
            store[i].in_use = 1;
            strncpy(store[i].key, key, MAX_KEY_LEN - 1);
            store[i].key[MAX_KEY_LEN - 1] = '\0';
            strncpy(store[i].value, value, MAX_VALUE_LEN - 1);
            store[i].value[MAX_VALUE_LEN - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

int get(const char *key, char *result) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (store[i].in_use && strcmp(store[i].key, key) == 0) {
            strncpy(result, store[i].value, MAX_VALUE_LEN);
            result[MAX_VALUE_LEN - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

int del(const char *key) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (store[i].in_use && strcmp(store[i].key, key) == 0) {
            store[i].in_use = 0;
            return 0;
        }
    }
    return -1;
}

int closeStore() {
    return 0;
}
