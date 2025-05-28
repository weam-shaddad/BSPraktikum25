#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "keyValStore.h"

#define PORT 5678
#define BUFFER_SIZE 1024

int main() {
    // Initialisiere den Shared Memory Key-Value Store
    if (initStore() != 0) {
        fprintf(stderr, "Shared Memory Initialisierung fehlgeschlagen.\n");
        exit(2);
    }

    // Erstelle den Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socketerstellung fehlgeschlagen");
        exit(2);
    }

    // Adresse konfigurieren
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    // Socket an Adresse binden
    if (bind(server_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Bind fehlgeschlagen");
        exit(2);
    }

    // Auf Verbindungen warten
    if (listen(server_fd, 3) < 0) {
        perror("Listen fehlgeschlagen");
        exit(2);
    }

    printf("Server hört auf Port %d...\n", PORT);

    // Akzeptiere eine Verbindung
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("Accept fehlgeschlagen");
        exit(2);
    }

    printf("Client verbunden!\n");

    // Buffer für die Kommunikation
    char buffer[BUFFER_SIZE];
    char line[BUFFER_SIZE];
    int line_pos = 0;
    memset(line, 0, sizeof(line));

    while (1) {
        int len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            printf("Verbindung getrennt.\n");
            break;
        }

        for (int i = 0; i < len; i++) {
            char ch = buffer[i];

            if (ch == '\r') continue;  // Manche Clients senden \r vor \n

            if (ch == '\n') {
                line[line_pos] = '\0';  // Zeilenende markieren
                printf("Empfangen: %s\n", line);

                // QUIT-Befehl erkennen
                if (strncmp(line, "QUIT", 4) == 0) {
                    send(client_fd, "Verbindung wird beendet.\n", 26, 0);
                    goto END;
                }

                // Befehle zerlegen
                char* command = strtok(line, " \n");
                char* key = strtok(NULL, " \n");
                char* value = strtok(NULL, " \n");

                if (command == NULL || key == NULL) {
                    send(client_fd, "Fehlerhafte Eingabe\n", 21, 0);
                }
                else if (strcmp(command, "PUT") == 0) {
                    if (value == NULL) {
                        send(client_fd, "PUT braucht key + value\n", 25, 0);
                    } else {
                        if (put(key, value) == 0) {
                            char msg[256];
                            snprintf(msg, sizeof(msg), "PUT:%s:%s\n", key, value);
                            send(client_fd, msg, strlen(msg), 0);
                        } else {
                            send(client_fd, "Speicher voll\n", 14, 0);
                        }
                    }
                }
                else if (strcmp(command, "GET") == 0) {
                    char result1[256];
                    if (get(key, result1) == 0) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "GET:%s:%s\n", key, result1);
                        send(client_fd, msg, strlen(msg), 0);
                    } else {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "GET:%s:key_nonexistent\n", key);
                        send(client_fd, msg, strlen(msg), 0);
                    }
                }
                else if (strcmp(command, "DEL") == 0) {
                    char msg[256];
                    if (del(key) == 0) {
                        snprintf(msg, sizeof(msg), "DEL:%s:key_deleted\n", key);
                    } else {
                        snprintf(msg, sizeof(msg), "DEL:%s:key_nonexistent\n", key);
                    }
                    send(client_fd, msg, strlen(msg), 0);
                }
                else {
                    send(client_fd, "Unbekannter Befehl\n", 20, 0);
                }

                // Zeile zurücksetzen
                line_pos = 0;
                memset(line, 0, sizeof(line));
            } else {
                if (line_pos < BUFFER_SIZE - 1) {
                    line[line_pos++] = ch;
                }
            }
        }
    }

END:
    close(client_fd);
    close(server_fd);

    // Bereinige den Shared Memory
    if (closeStore() != 0) {
        fprintf(stderr, "Shared Memory Bereinigung fehlgeschlagen.\n");
    }

    return 0;
}
