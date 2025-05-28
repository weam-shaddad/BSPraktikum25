#ifndef KEYVALSTORE_H
#define KEYVALSTORE_H

// Initialisiert den Shared Memory Key-Value Store
int initStore();

// Bereinigt den Shared Memory (munmap und unlink)
int closeStore();

// Legt einen Eintrag an oder aktualisiert ihn
int put(const char *key, const char *value);

// Liest den Wert zu einem gegebenen Schlüssel aus
// Ist der Schlüssel vorhanden, wird value befüllt und 0 zurückgegeben, sonst -1.
int get(const char *key, char *value);

// Löscht einen Eintrag; bei Erfolg wird 0 und sonst -1 zurückgegeben.
int del(const char *key);

#endif
