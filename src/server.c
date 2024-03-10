#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "defines.h"

int main() {
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family      = AF_INET;
    server_address.sin_port        = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    listen(server_socket_fd, 1);

    int client_socket_fd = accept(server_socket_fd, NULL, NULL);

    char message[MAX_LINE_SIZE];
    recv(client_socket_fd, message, MAX_LINE_SIZE, 0);
    send(client_socket_fd, message, MAX_LINE_SIZE, 0);

    close(client_socket_fd);
    close(server_socket_fd);

    return 0;
}