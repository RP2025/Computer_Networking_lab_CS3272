#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1234
#define MAX_MESSAGE_SIZE 1024

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    char message[MAX_MESSAGE_SIZE];

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept connection from the client
    addr_size = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
    if (client_socket == -1) {
        perror("Error accepting connection");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Receive message from client
        recv(client_socket, message, sizeof(message), 0);
        printf("Client: %s\n", message);

        // Check if the client wants to end the chat
        if (strcmp(message, "Bye") == 0) {
            printf("Chat session closed by client.\n");
            break;
        }

        // Get the server's response
        printf("Server: ");
        fgets(message, sizeof(message), stdin);

        // Send the response back to the client
        send(client_socket, message, sizeof(message), 0);

        // Check if the server wants to end the chat
        if (strcmp(message, "Bye") == 0) {
            printf("Chat session closed by server.\n");
            break;
        }
    }

    // Close the sockets
    close(client_socket);
    close(server_socket);

    return 0;
}
