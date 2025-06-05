#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include "keyValStore.h"

#define PORT 5678
#define BUFFER_SIZE 1024

void handleClient(int client_fd);

#endif