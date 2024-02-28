#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "10.2.1.40"  // Change this to the server's IP address
#define PORT 12345
#define BUFFER_SIZE 1024

void *send_message(void *arg);
void *receive_message(void *arg);

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t send_tid, recv_tid;
    char buffer[BUFFER_SIZE];

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    // Create threads for sending and receiving messages
    pthread_create(&send_tid, NULL, send_message, (void *)&client_socket);
    pthread_create(&recv_tid, NULL, receive_message, (void *)&client_socket);

    // Wait for threads to finish
    pthread_join(send_tid, NULL);
    pthread_join(recv_tid, NULL);

    // Close the client socket
    close(client_socket);

    return 0;
}

void *send_message(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];

while (1) {
        // Get user input
        printf("You: ");
        fgets(buffer, sizeof(buffer), stdin);

        // Send message to the server
        send(client_socket, buffer, strlen(buffer), 0);

        // Check if the user wants to end the chat
        if (strcmp(buffer, "Bye\n") == 0) {
            break;
        }
    }

    pthread_exit(NULL);
}

void *receive_message(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        // Receive message from server
        ssize_t recv_size = recv(client_socket, buffer, sizeof(buffer), 0);
        if (recv_size <= 0) {
            // Server disconnected or error
            break;
        }

        // Print received message
        printf("Server: %s", buffer);

        // Check if the server wants to end the chat
        if (strcmp(buffer, "Bye\n") == 0) {
            break;
        }
    }

    pthread_exit(NULL);
}

