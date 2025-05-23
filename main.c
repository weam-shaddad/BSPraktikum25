#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "keyValStore.h"

#define PORT 5678
#define BUFFER_SIZE 1024

int main() {
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(2);
    }

    // Create address structure
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    int result = bind(server_fd, (struct sockaddr*)&server, sizeof(server));
    if (result < 0) {
        perror("Bind failed");
        exit(2);
    }

    // Listen for connections
    int ret = listen(server_fd, 3);
    if (ret < 0) {
        perror("Listen failed");
        exit(2);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept a connection
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("Accept failed");
        exit(2);
    }

    printf("Client connected!\n");

    // Buffer für Kommunikation
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

            if (ch == '\r') continue;  // Telnet schickt manchmal \r vor \n

            if (ch == '\n') {
                line[line_pos] = '\0';  // Null-terminieren
                printf("Empfangen: %s\n", line);

                // QUIT erkennen
                if (strncmp(line, "QUIT", 4) == 0) {
                    send(client_fd, "Verbindung wird beendet.\n", 26, 0);
                    goto END;
                }

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
                        put(key, value);
                        char msg[256];
                        snprintf(msg, sizeof(msg), "PUT:%s:%s\n", key, value);
                        send(client_fd, msg, strlen(msg), 0);
                    }
                }
                else if (strcmp(command, "GET") == 0) {
                    char result[256];
                    if (get(key, result) == 0) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "GET:%s:%s\n", key, result);
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
}