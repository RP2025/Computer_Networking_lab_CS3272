#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 1234
#define MAX_MESSAGE_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    char message[MAX_MESSAGE_SIZE];

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Connect to localhost

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Get message from user
        printf("Client: ");
        fgets(message, sizeof(message), stdin);

        // Send message to server
        send(client_socket, message, sizeof(message), 0);

        // Check if the client wants to end the chat
        if (strcmp(message, "Bye") == 0) {
            printf("Chat session closed by client.\n");
            break;
        }

        // Receive response from server
        recv(client_socket, message, sizeof(message), 0);
        printf("Server: %s\n", message);

        // Check if the server wants to end the chat
        if (strcmp(message, "Bye") == 0) {
            printf("Chat session closed by server.\n");
            break;
        }
    }

    // Close the socket
    close(client_socket);

    return 0;
}

