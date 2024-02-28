#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define BUFFER_SIZE 1024

void *handle_client(void *arg);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t tid;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, 5) == -1) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        // Create a new thread to handle the client
        if (pthread_create(&tid, NULL, handle_client, (void *)&client_socket) != 0) {
            perror("Error creating thread");
            close(client_socket);
            continue;
        }
    }

    // Close the server socket (this won't be reached in this simple example)
    close(server_socket);

    return 0;
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        // Receive message from client
        ssize_t recv_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_size <= 0) {
            if (recv_size == 0) {
                // Client disconnected
                printf("Client disconnected.\n");
            } else {
                perror("Error receiving message from client");
            }
            break;
        }

        // Print received message
        printf("Client: %s\n", buffer);

        // Send response back to the client
        send(client_socket, buffer, recv_size, 0);

        // Check if the client wants to end the chat
        if (strcmp(buffer, "Bye") == 0) {
            break;
        }
    }

    // Close the client socket
    close(client_socket);

    pthread_exit(NULL);
}

