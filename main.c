#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include "keyValStore.h"

#define PORT 5678
#define BUFFER_SIZE 1024

void handleClient(int client_fd) {
    char welcome_message[] = "Mit Server verbunden!\n";
    send(client_fd, welcome_message, strlen(welcome_message), 0);

    char recvBuf[BUFFER_SIZE];
    while (1) {
        int len = recv(client_fd, recvBuf, BUFFER_SIZE - 1, 0);
        if (len <= 0)
            break;
        recvBuf[len] = '\0';

        char *line = strtok(recvBuf, "\n");
        while (line != NULL) {
            size_t lineLength = strlen(line);
            if (lineLength > 0 && line[lineLength - 1] == '\r')
                line[lineLength - 1] = '\0';

            if (strncmp(line, "QUIT", 4) == 0) {
                send(client_fd, "Beende Verbindung\n", strlen("Beende Verbindung\n"), 0);
                return;
            }

            char commandCopy[BUFFER_SIZE];
            strncpy(commandCopy, line, BUFFER_SIZE);
            commandCopy[BUFFER_SIZE - 1] = '\0';

            char *command = strtok(commandCopy, " ");
            char *key = strtok(NULL, " ");
            char *value = strtok(NULL, " ");

            if (command == NULL || key == NULL) {
                send(client_fd, "Fehlerhafte Eingabe\n", strlen("Fehlerhafte Eingabe\n"), 0);
            }
            else if (strcmp(command, "PUT") == 0) {
                if (value == NULL) {
                    send(client_fd, "PUT braucht key + value\n", strlen("PUT braucht key + value\n"), 0);
                } else {
                    if (put(key, value) == 0) {
                        char response[256];
                        snprintf(response, sizeof(response), "PUT:%s:%s\n", key, value);
                        send(client_fd, response, strlen(response), 0);
                    } else {
                        send(client_fd, "Speicher voll\n", strlen("Speicher voll\n"), 0);
                    }
                }
            }
            else if (strcmp(command, "GET") == 0) {
                char result[256];
                if (get(key, result) == 0) {
                    char response[256];
                    snprintf(response, sizeof(response), "GET:%s:%s\n", key, result);
                    send(client_fd, response, strlen(response), 0);
                } else {
                    char response[256];
                    snprintf(response, sizeof(response), "GET:%s:key_nonexistent\n", key);
                    send(client_fd, response, strlen(response), 0);
                }
            }
            else if (strcmp(command, "DEL") == 0) {
                char response[256];
                if (del(key) == 0) {
                    snprintf(response, sizeof(response), "DEL:%s:key_deleted\n", key);
                } else {
                    snprintf(response, sizeof(response), "DEL:%s:key_nonexistent\n", key);
                }
                send(client_fd, response, strlen(response), 0);
            }
            else {
                send(client_fd, "Unbekannter Befehl\n", strlen("Unbekannter Befehl\n"), 0);
            }

            line = strtok(NULL, "\n");
        }
    }
    close(client_fd);
}

int main() {
    if (initStore() != 0) {
        fprintf(stderr, "Initialisierung des Key–Value Stores fehlgeschlagen.\n");
        exit(EXIT_FAILURE);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socketerstellung fehlgeschlagen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Bind fehlgeschlagen");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen fehlgeschlagen");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);
    printf("Server läuft auf Port %d...\n", PORT);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
            continue;

        printf("Server: Ein Client hat sich verbunden (FD: %d)\n", client_fd);

        if (fork() == 0) {
            close(server_fd);
            handleClient(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    close(server_fd);
    if (closeStore() != 0) {
        fprintf(stderr, "Bereinigung des Key–Value Stores fehlgeschlagen.\n");
    }

    return 0;
}
