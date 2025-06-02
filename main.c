#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5678


int main() {
    if (initStore() != 0) {
        fprintf(stderr, "Shared Memory Fehler!\n");
        exit(1);
    }

    start_server(); // Ruft server_fd auf + startet Forks
    return 0;
}
