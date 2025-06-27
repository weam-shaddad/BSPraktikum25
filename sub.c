
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>         // für close()
#include <arpa/inet.h>      // für Strukturen von: inet_addr, htons, etc.
#include <sys/socket.h>     // für Funktionen von: socket(), bind(), listen(), etc.
#include "keyValStore.h"
#include "sub.h"

// headers für semaphore
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

// pub/sub
#define max_subs 100

struct Subscription {
    int client_fd; // client verbindungs-id
    char key[128]; // den key, für den sich der Client angemeldet hat
};

struct Subscription subscriptions[max_subs]; // das array für die Subscriptions Einträgen
int subs_count = 0; // wird erhöht, wenn jemand aboniert/subs-array verwendet
void notify_subscribers(const char *key, const char *message);

// transaktionen:

union semun { // a box that holds one of several types of data
    int val; // value of a semaphore (SETVAL)
    struct semid_ds *buf; // ipc_stat, ipc_set
    unsigned short *array; // set/get multiple semaphores (Setall, getall)
};

int sem_id; // semaphore ID

#define PORT 5678
#define BUFFER_SIZE 1024


void start_server() {

    printf("Server wurde hier gestartet...\n");

    // hier wird ein semaphore erstellt, ein semaphore gruppe mit einem semaphore drin, ohne values
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666); // semaphore gruppe mit einen semaphore, 0666 -> read/write premission
    // Fehlerprüfung:
    if (sem_id == -1) {
        perror("semget failed");
        exit(1);
    }
    union semun arg; // neue variabel, von typ union semun
    // das semaphore muss auf 1 starten, wenn es auf 0 startet, bleibt er locked und unutzbar
    arg.val = 1; // 1 = unlocked , available for use
    // semctl hilft uns, den Semaphore zu kontrollieren
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        perror("semctl failed");
        exit(1);
    }
    int server_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        return;
    }
    listen(server_fd, SOMAXCONN);

    printf("Server gestartet. Warte auf Verbindung auf Port %d...\n", PORT);
    printf(">> Starte jetzt start_multiclient_server()\n");

    start_multiclient_server(server_fd);
}
void start_multiclient_server(int server_fd) {

    printf("Multiclient-Server bereit...\n");

    int client_count = 0;

    while (1) {
        printf("Warte auf neuen Client... \n");

        int client_fd = accept(server_fd, NULL, NULL);
        printf(" accept() ausgeführt \n");

        // accept wartet auf eine neue eingehende Verbindung
        // Fehlerprüfung:
        if (client_fd < 0) {
            perror("accept fehlgeschlagen");
            continue;
        }
        client_count++; // zum testing
        printf("Client %d verbunden. FD: %d\n", client_count, client_fd);

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

// Funktion für die Transaktionen
int is_store_locked() {
    struct sembuf test = {0, -1, IPC_NOWAIT}; // try to lock, dont wait and test

    int result = semop(sem_id, &test, 1);
    if (result == -1) {
        // store is locked!
        return 1;
    }
    struct sembuf undo = {0, 1, 0};
    semop(sem_id, &undo, 1);
    return 0;
}

// Funktion für die Sub/Pub

// wir müssen erst die client_fd verarbeit
// wer welche Schlüssel "abonniert" hat, zu speicher
// nachrichten an Clients schicken

void register_sub(int client_fd, const char *key) {
    if (subs_count >= max_subs) {
        send(client_fd, "Maximal Anzahl an Subs erreicht\n", 40, 0);
        return;
        }

    // um Duplikate zu vermeiden
    for (int i = 0; i < subs_count; i++) {
        if (subscriptions[i].client_fd == client_fd &&
            strcmp(subscriptions[i].key, key) == 0) {
            send(client_fd, "Du bist bereits für diesen Key registriert\n", 44, 0);
            return;
            }
    }
    printf("→ Registriere SUB: client_fd = %d, key = %s\n", client_fd, key);

    // den übergebenen key in die subs liste kopieren
    strncpy(subscriptions[subs_count].key, key, sizeof(subscriptions[subs_count].key));
    // \0 sollte am ende sein. -> gültige Zeichenkette
    subscriptions[subs_count].key[sizeof(subscriptions[subs_count].key) - 1] = '\0';
    // Speichert den Client, der den Sub befehl geschickt hat
    subscriptions[subs_count].client_fd = client_fd;
    subs_count++;

    printf("Abonnierte Keys:\n");
    for (int i = 0; i < subs_count; i++) {
        printf("  [%d] FD=%d, Key=%s\n", i, subscriptions[i].client_fd, subscriptions[i].key);
    }
}

void notify_subscribers(const char *key, const char *message) {
    for (int i = 0; i < subs_count; i++) {
        if (strcmp(subscriptions[i].key, key) == 0) {
            printf("→ Benachrichtige Client %d für Key %s\n", subscriptions[i].client_fd, key);
            ssize_t result = send(subscriptions[i].client_fd, message, strlen(message), 0);

            if (result == -1) {
                perror("Fehler beim Senden an Subscriber");
            }
        }    }
}

void handleClient(int client_fd) {

    char line[BUFFER_SIZE];
    int line_pos = 0;
    int in_transaction = 0;

    send(client_fd, "Verbunden. \n", 13, 0);

    while (1) {
        char ch;
        int len = recv(client_fd, &ch, 1, 0); // nur 1 Zeichen lesen

        /* wir wollen erstmal die Nachrichten vom Client lesen
        die Daten vom Client sind über "client_fd" verbunden
        sie sind dann im "buffer" gespeichert (buffer von typ char[])
        -1 -> wir lassen 1 platz frei, 0 -> blockiert bis daten kommen
        */

        if (len <= 0) {// was passiert wenn keine Daten übermittelt sind? 0-> Verbindung beendet, <0 -> Fehler ist aufetreten
            printf("Client getrennt/Fehler \n");
            break;
        }
        if (ch == '\r') continue;  // \r ignorieren

        if (ch == '\n') {
            line[line_pos] = '\0';
            printf("Empfangen: %s\n", line);

            // Befehle:
            char *command = strtok(line, " \n"); // PUT;GET;DEL
            char *key = strtok(NULL, " \n");
            char *value = strtok(NULL, " \n");

            if (command) {
                for (char *p = command; *p; ++p) {
                    *p = toupper((unsigned char)*p);  // Großbuchstaben erzwingen
                }
            }

            // Debug-Ausgabe:
            printf("⤷ command='%s', key='%s', value='%s'\n",
                   command ? command : "NULL",
                   key ? key : "NULL",
                   value ? value : "NULL");

            if (command == NULL) {
                send(client_fd, "Ungültiger Befehl\n", 18, 0);
                // continue;
            }

            // Transaktionen:
            // bevor wir mit PUT GET DEL arbeiten, müssen wir erstmal checken, ob das Client in einem Transaktion ist, oder ist diese Transaktion in Nutzung von einer anderen Client
            else if (strcmp(command, "BEG") == 0) {
                if (key != NULL || value != NULL) {
                    send(client_fd, "BEG erwartet keine Argumente\n", 30, 0);
                }
                else if (in_transaction) {
                    send(client_fd, "Du bist bereits in einer Transaktion\n", 37, 0);
                }
                else {
                    struct sembuf down = {0, -1, 0};
                    if (semop(sem_id, &down, 1) == -1) {
                        perror("semop BEG failed");
                        send(client_fd, "Transaktion konnte nicht gestartet werden\n", 43, 0);
                    }
                    else {
                        in_transaction = 1;
                        send(client_fd, "Transaktion gestartet\n", 23, 0);
                    }
                }
            }

            else if (strcmp(command, "END") == 0) {
                if (!in_transaction) {
                    send(client_fd, "Du bist in keiner Transaktion \n", 30, 0);
                }
                struct sembuf up = {.sem_num = 0, .sem_op = 1, .sem_flg = 0}; // unlock

                if (semop(sem_id, &up, 1) == -1) {
                    perror("semop END failed");
                    send(client_fd, "Fehler beim Beenden der Transaktion\n", 37, 0);
                }
                in_transaction = 0;
                send(client_fd, "Transaktion beendet\n", 21, 0);
            }
            // Pub/Sub Implementierung
            else if (strcmp(command, "SUB") == 0) {
                if (key == NULL) {
                    send(client_fd, "SUB braucht einen key!\n", 24, 0);
                } else {
                    register_sub(client_fd, key);

                    char result[256];
                    char response[256];

                    if (get(key, result) == 0) {
                        snprintf(response, sizeof(response), "SUB:%s:%s\n", key, result);
                    } else {
                        snprintf(response, sizeof(response), "SUB:%s:key_nonexistent\n", key);
                    }

                    send(client_fd, response, strlen(response), 0);
                }
            }



            // Meilenstein 1+2

            // QUIT
            else if (strcmp(command, "QUIT") == 0) {
                send(client_fd, "Verbindung wird beendet.\n", 26, 0);
                break;
            }

            // PUT

            else if (strcmp(command, "PUT") == 0) {
                if (!in_transaction && is_store_locked()) {
                    send(client_fd, "Store ist aktuell durch eine Transaktion gesperrt\n", 50, 0);
                    // continue;
                }

                if (key == NULL || value == NULL) {
                    printf("command='%s', key='%s', value='%s'\n", command, key, value);
                    send(client_fd, "PUT braucht key UND value\n", 28, 0);
                    // continue;
                }

                if (put(key, value) == 0) {
                    char response[256];
                    snprintf(response, sizeof(response), "PUT:%s:%s\n", key, value);
                    send(client_fd, response, strlen(response), 0);
                    notify_subscribers(key, response);
                } else {
                    send(client_fd, "Fehler beim Speichern\n", 23, 0);
                }

                // GET

            } else if (strcmp(command, "GET") == 0) {
                if (!in_transaction && is_store_locked()) {
                    send(client_fd, "Store ist aktuell durch eine Transaktion gesperrt\n", 50, 0);
                    // continue;
                }

                if (key == NULL) {
                    send(client_fd, "GET braucht einen key\n", 23, 0);
                    // continue;
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
                if (!in_transaction && is_store_locked()) {
                    send(client_fd, "Store ist aktuell durch eine Transaktion gesperrt\n", 50, 0);
                    // continue;
                }

                if (key == NULL) { // kein Key gegeben
                    send(client_fd, "DEL braucht einen key\n", 23, 0);
                    // continue;
                }

                if (del(key) == 0) {
                    char response[256];
                    snprintf(response, sizeof(response), "DEL:%s:key_deleted\n", key);
                    send(client_fd, response, strlen(response), 0);
                    notify_subscribers(key, response);
                } else {
                    char response[256];
                    snprintf(response, sizeof(response), "DEL:%s:key_nonexistent\n", key);
                    send(client_fd, response, strlen(response), 0);
                }
            }else {
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

    close(client_fd);
}


