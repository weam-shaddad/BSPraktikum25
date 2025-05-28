#include "keyValStore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>   // mmap, munmap
#include <fcntl.h>      // shm_open, O_CREAT, O_RDWR
#include <unistd.h>     // ftruncate

// Definition der Grenzen
#define MAX_PAIRS       100
#define MAX_KEY_LENGTH  64
#define MAX_VALUE_LENGTH 256
#define SHM_NAME        "/kvstore_shm"

// Struktur für ein Key-Value-Paar
typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int in_use;  // 0 = frei, 1 = belegt
} KeyValuePair;

// Der gesamte Schlüsselwertspeicher
typedef struct {
    KeyValuePair pairs[MAX_PAIRS];
} KVStore;

// Globale Variablen für Shared Memory
static KVStore *store = NULL;
static int shm_fd = -1;

int initStore() {
    // Erstelle bzw. öffne das Shared-Memory-Objekt
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open fehlgeschlagen");
        return -1;
    }
    // Größe des Shared Memory festlegen
    if (ftruncate(shm_fd, sizeof(KVStore)) < 0) {
        perror("ftruncate fehlgeschlagen");
        return -1;
    }
    // Mappe den Shared Memory in den Adressraum
    store = mmap(NULL, sizeof(KVStore), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (store == MAP_FAILED) {
        perror("mmap fehlgeschlagen");
        return -1;
    }
    // Initialisiere alle Einträge als frei
    for (int i = 0; i < MAX_PAIRS; i++) {
        store->pairs[i].in_use = 0;
    }
    return 0;
}

int closeStore() {
    // Entferne das Mapping
    if (munmap(store, sizeof(KVStore)) < 0) {
        perror("munmap fehlgeschlagen");
        return -1;
    }
    // Entferne das Shared Memory Objekt
    if (shm_unlink(SHM_NAME) < 0) {
        perror("shm_unlink fehlgeschlagen");
        return -1;
    }
    return 0;
}

int put(const char *key, const char *value) {
    // Falls der Schlüssel bereits existiert, aktualisieren wir den Wert.
    for (int i = 0; i < MAX_PAIRS; i++) {
        if (store->pairs[i].in_use && strncmp(store->pairs[i].key, key, MAX_KEY_LENGTH) == 0) {
            strncpy(store->pairs[i].value, value, MAX_VALUE_LENGTH - 1);
            store->pairs[i].value[MAX_VALUE_LENGTH - 1] = '\0';
            return 0;
        }
    }
    // Falls der Schlüssel nicht existiert, suche einen freien Eintrag.
    for (int i = 0; i < MAX_PAIRS; i++) {
        if (!store->pairs[i].in_use) {
            store->pairs[i].in_use = 1;
            strncpy(store->pairs[i].key, key, MAX_KEY_LENGTH - 1);
            store->pairs[i].key[MAX_KEY_LENGTH - 1] = '\0';
            strncpy(store->pairs[i].value, value, MAX_VALUE_LENGTH - 1);
            store->pairs[i].value[MAX_VALUE_LENGTH - 1] = '\0';
            return 0;
        }
    }
    // Kein freier Platz vorhanden
    return -1;
}

int get(const char *key, char *value) {
    for (int i = 0; i < MAX_PAIRS; i++) {
        if (store->pairs[i].in_use && strncmp(store->pairs[i].key, key, MAX_KEY_LENGTH) == 0) {
            strncpy(value, store->pairs[i].value, MAX_VALUE_LENGTH);
            return 0;
        }
    }
    return -1;
}

int del(const char *key) {
    for (int i = 0; i < MAX_PAIRS; i++) {
        if (store->pairs[i].in_use && strncmp(store->pairs[i].key, key, MAX_KEY_LENGTH) == 0) {
            store->pairs[i].in_use = 0;
            return 0;
        }
    }
    return -1;
}
