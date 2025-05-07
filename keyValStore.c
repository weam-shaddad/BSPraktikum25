//
// Created by weams on 07/05/2025.
//
#include "keyValStore.h"

#include <stdio.h>
#include <string.h>

static KeyValuePair store[MAX_STORE_SIZE];
static int store_size = 0;

int put(char* key, char* value) {
    // 1. Existriert der key schon?
    for (int i=0; i<store_size; i++) {
        if (strcmp(key, store[i].key) == 0) {
            // String Vergleich/compare
            // key gefundey und existiert, müssen nur übergeschrieben
            strncpy(store[i].value, value, sizeof(store[i].value) - 1); // -1 -> nur 255 zeichnen maximal kopieren
            store[i].value[sizeof(store[i].value) - 1] = '\0'; // Sicherheit
            return 0;
        } }
    // nicht gefunden
    if (store_size >= MAX_STORE_SIZE) { // ob array schon voll ist
        return -1;  // kein platz mehr
    }
    // wenn doch Speicherplatz gibt -> neues paar speicher, store_size erhöhen, return 0
    strncpy(store[store_size].key, key, sizeof(store[store_size].key) - 1);
    store[store_size].key[sizeof(store[store_size].key) - 1] = '\0';
    // store[store_size] -> das nächste freie Element im speicher
    // store[store_size].key[255] = '\0'; -> string sicher terminiert ist

    strncpy(store[store_size].value, value, sizeof(store[store_size].value) - 1);
    store[store_size].value[sizeof(store[store_size].value) - 1] = '\0';

    store_size++; // anzahl gespeicherter paare erhöhen
    return 0;

}

int get(char* key, char* res) {
    // alle gespeicherten paare durchsuchen
    for (int i=0; i<store_size; i++) {
        if (strcmp(store[i].key, key) == 0) { // vergleichen den gesuchten key mit dem gespeicherten
            // char *strncpy(char *dest, const char *src, size_t n);
            strncpy(res, store[i].value, sizeof(store[i].value)); // kopieren den gefunden value in den speicher
            res[sizeof(store[i].value) -1] = '\0';
            return 0;
        }
    }
    // key nicht gefunden
    return -1;
}

int del(char* key) {
    for (int i = 0; i < store_size; i++) {
        if (strcmp(store[i].key, key)== 0) {
            // löschen durch überschreiben
            store[i] = store[store_size-1]; // letzte eintrag in die lücke kopieren
            store_size--; // anzahl reduzieren -> löschen
            return 0;
        }
        }
    return -1;
    }

