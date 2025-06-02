#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5678


int main() {

    start_server();  // Diese Funktion machst du in sub.c

    return 0;
}
