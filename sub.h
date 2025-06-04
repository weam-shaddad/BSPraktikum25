#ifndef SUB_H
#define SUB_H

void start_server();
void start_multiclient_server(int server_fd);
void handleClient(int client_fd);

#endif //SUB_H
