//
// Created by weams on 07/05/2025.
//
#include <stdio.h>
#include "sub.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>         // für close()
#include <arpa/inet.h>      // für Strukturen von: inet_addr, htons, etc.
#include <sys/socket.h>     // für Funktionen von: socket(), bind(), listen(), etc.

#include "keyValStore.h"

#define PORT 5678
#define BUFFER_SIZE 1024


void start_server() {
    printf("Server wurde hier gestartet...\n");

    int server_fd, client_fd;

    struct sockaddr_in client_addr;

    char buffer[BUFFER_SIZE];

    socklen_t client_len = sizeof(client_addr);

    // 1.  Anlegen eines Sockets / Socket erstellen
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Protokoll-Familie: AF_NET, Kommunikationstyp: SOCK_STREAM -> protocol-parameter ist dann 0
    if (server_fd < 0) {
        perror("socket");
        return;
    }

    // 2. Binden einer Adresse an das Socket
    // erstmal Adresse konfigurieren
    struct sockaddr_in server_addr; // ein struct für die Adresse
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // alle interfaces
    server_addr.sin_port = htons(PORT); // port  5678, oben definiert

    // bind() funktion: verbindet die Socket mit einer IP-Adresse und einem Port
    // bind() kann fehlschlagen...
    // bind( sock, (struct sockaddr*) &server, sizeof(server));
    // sock: server_fd, server:S server_addr, sizeof: der server size
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        return;
    }

    // Auf Verbindung warten
    listen(server_fd, 5); // max 1 wartender Client
    printf("Server gestartet. Warte auf Verbindung auf Port %d...\n", PORT);

    /*
    Wenn wir mit Multiclient server arbeiten möchten:
     */

    start_multiclient_server(server_fd);
    close(server_fd);

    /*
    Wenn wir ohne Multiclient arbeiten:
     */
    /*
    // Client akzeptieren
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len); //
    if (client_fd < 0) {
        perror("accept error");
        return;
    }
    printf("Client verbunden.\n");

    char line[BUFFER_SIZE];
    int line_pos = 0;

    while (1) {
        int len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            printf("Verbindung getrennt. \n");
            break;
        }

        for (int i = 0; i < len; i++) { // for loop damit wir
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
                char* key     = strtok(NULL, " \n");
                char* value   = strtok(NULL, " \n");

                if (command == NULL || key == NULL) {
                    send(client_fd, "Fehlerhafte Eingabe\n", 21, 0);

                }
                else if (strcmp(command, "PUT") == 0) { // put command
                    if (value == NULL) {
                        send(client_fd, "PUT braucht key + value\n", 25, 0);
                    } else {
                        put(key, value);
                        char msg[256];
                        snprintf(msg, sizeof(msg), "PUT:%s:%s\n", key, value);
                        send(client_fd, msg, strlen(msg), 0); // key senden
                    }

                } else if (strcmp(command, "GET") == 0) { // get command
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

                } else if (strcmp(command, "DEL") == 0) {
                    char msg[256];
                    if (del(key) == 0) {
                        snprintf(msg, sizeof(msg), "DEL:%s:key_deleted\n", key);
                    } else {
                        snprintf(msg, sizeof(msg), "DEL:%s:key_nonexistent\n", key);
                    }
                    send(client_fd, msg, strlen(msg), 0);
                } else {
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

    // 8. Verbindung schließen
    END:
    close(client_fd);
    close(server_fd);

}
*/
}
void start_multiclient_server(int server_fd) {

    printf("Multiclient-Server bereit...\n");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        // accept wartet auf eine neue eingehende Verbindung
        // Fehlerbeheben:
        if (client_fd < 0) continue;

        printf("Client verbunden. FD: %d\n", client_fd);


        // Fork, damit jeder Client in einem eigenen Prozess abgearbeitet wird
        if (fork() == 0) { // exakte kopie des laufenden Prozess, wenn ==0 sind wir im Kindprozess
            // im Kindprozess
            close(server_fd);
            handleClient(client_fd); // die Verarbeitung den Client, also put,get,del,quit..
            close(client_fd);
            exit(0);
        }

        close(client_fd); // Der Elternprozess braucht die Client Verbindung nicht, das macht der Kindprozess...

    }
}

