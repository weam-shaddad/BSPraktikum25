//
// Created by weams on 07/05/2025.
//

#ifndef KEVVALSTORE_H
#define KEVVALSTORE_H

#define MAX_STORE_SIZE 100

typedef struct {
    char key[256];
    char value[256];
} KeyValuePair;

int put(char* key, char* value);
int get(char* key, char* value);
int del(char* key);

#endif //KEVVALSTORE_H
