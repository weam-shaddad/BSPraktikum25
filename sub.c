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
    setbuf(stdout, NULL); // Deaktiviert stdout-Pufferung

    printf("Server wurde hier gestartet...\n");

    int server_fd, client_fd;

    struct sockaddr_in client_addr;

    char buffer[BUFFER_SIZE];

    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

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
    listen(server_fd, SOMAXCONN); // max 2 wartender Client
    printf("Server gestartet. Warte auf Verbindung auf Port %d...\n", PORT);

    printf(">> Starte jetzt start_multiclient_server()\n");

    fflush(stdout);

    start_multiclient_server(server_fd);

}
void start_multiclient_server(int server_fd) {

    printf("Multiclient-Server bereit...\n");
    fflush(stdout);

    int client_count = 0;

    while (1) {
        printf("Warte auf neuen Client... \n");
        fflush(stdout);

        int client_fd = accept(server_fd, NULL, NULL);
        printf(" accept() ausgeführt \n");

        // accept wartet auf eine neue eingehende Verbindung
        // Fehlerbeheben:
        if (client_fd < 0) {
            perror("accept fehlgeschlagen");
            continue;
        }
        client_count++;
        printf("Client %d verbunden. FD: %d\n", client_count, client_fd);
        fflush(stdout);

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        }
        // Fork, damit jeder Client in einem eigenen Prozess abgearbeitet wird
        if (pid == 0) { // exakte kopie des laufenden Prozess, wenn ==0 sind wir im Kindprozess
            // im Kindprozess
            close(server_fd);
            handleClient(client_fd); // die Verarbeitung den Client, also put,get,del,quit..
            close(client_fd);
            exit(0);
        }

        close(client_fd); // Der Elternprozess braucht die Client Verbindung nicht, das macht der Kindprozess...

    }

}

void handleClient(int client_fd) {
    char buffer[BUFFER_SIZE];

    send(client_fd, "Verbunden. \n", 13, 0);

    while (1) {

        int len = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        memset(buffer, 0, BUFFER_SIZE);

        /* wir wollen erstmal die Nachrichten vom Client lesen
        die Daten vom Client sind über "client_fd" verbunden
        sie sind dann im "buffer" gespeichert (buffer von typ char[])
        -1 -> wir lassen 1 platz frei, 0 -> blockiert bis daten kommen
        */

        if (len <= 0) {// was passiert wenn keine Daten übermittelt sind? 0-> Verbindung beendet, <0 -> Fehler ist aufetreten
            printf("Client getrennt/Fehler \n");
            sleep(1);

            break;
    }

        buffer[len] = '\0'; // Arrays in C brauchen \0 am ende
        printf("Empfangen: %s\n", buffer);

        // Befehle:
    char *command = strtok(buffer, " \n"); // PUT;GET;DEL
    char *key = strtok(NULL, " \n");
    char *value = strtok(NULL, " \n");

    if (command == NULL) {
        send(client_fd, "Ungültiger Befehl\n", 18, 0);
        continue;
    }

    // QUIT
    if (strcmp(command, "QUIT") == 0) {
        send(client_fd, "Verbindung wird beendet.\n", 26, 0);
        break;
    }

    // PUT

    if (strcmp(command, "PUT") == 0) {
        if (key == NULL || value == NULL) {
            send(client_fd, "PUT braucht key UND value\n", 28, 0);
            continue;
        }

        put(key, value); // put funktion aufrufen, Wert überschrieben wenn nötig
        // jetzt ist ein Puffer (zwischenspeicher) gebraucht
        char response[256]; // hier wird die Antwort an den Client gespeichert
        snprintf(response, sizeof(response), "PUT:%s:%s\n", key, value);
        send(client_fd, response, strlen(response), 0); // ergebnis senden


    // GET

    } else if (strcmp(command, "GET") == 0) {
        if (key == NULL) {
            send(client_fd, "GET braucht einen key\n", 23, 0);
            continue;
        }

        char result[256];

        if (get(key, result) == 0) { //key existiert
            char response[256];
            snprintf(response, sizeof(response), "GET:%s:%s\n", key, result);
            send(client_fd, response, strlen(response), 0);
        }
            else { // key existiert nicht
            char response[256];
            snprintf(response, sizeof(response), "GET:%s:key_nonexistent\n", key);
            send(client_fd, response, strlen(response), 0);
        }

        // DELETE

    } else if (strcmp(command, "DEL") == 0) {
        if (key == NULL) { // kein Key gegeben
            send(client_fd, "DEL braucht einen key\n", 23, 0);
            continue;
        }

        char response[256];

        if (del(key) == 0) {
            snprintf(response, sizeof(response), "DEL:%s:key_deleted\n", key);
        } else {
            snprintf(response, sizeof(response), "DEL:%s:key_nonexistent\n", key);
        }
        send(client_fd, response, strlen(response), 0);

    } else {
        send(client_fd, "Unbekannter Befehl\n", 20, 0);
    }
}

    close(client_fd);
}


