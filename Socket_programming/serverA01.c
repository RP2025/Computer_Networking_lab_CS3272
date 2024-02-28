#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in server_address;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8080...\n");

    // Accept connection from client
    if ((new_socket = accept(server_fd, NULL, NULL)) == -1) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_BUFFER_SIZE];
    ssize_t valread;

    // Receive and respond to messages until "Bye" is received
    while (1) {
        valread = read(new_socket, buffer, sizeof(buffer));
        printf("Client: %s\n", buffer);

        if (strcmp(buffer, "Bye") == 0) {
            printf("Chat session terminated by client.\n");
            break;
        }

        // Respond with the same message
        send(new_socket, buffer, strlen(buffer), 0);
        printf("Server: %s\n", buffer);
    }

    // Close sockets
    close(new_socket);
    close(server_fd);

    return 0;
}


