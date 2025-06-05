#ifndef KEYVALSTORE_H
#define KEYVALSTORE_H

#include <string.h>
#include <stdio.h>

#define MAX_ENTRIES 100
#define MAX_KEY_LEN 64
#define MAX_VALUE_LEN 192

int initStore();
int put(const char *key, const char *value);
int get(const char *key, char *result);
int del(const char *key);
int closeStore();

#endif