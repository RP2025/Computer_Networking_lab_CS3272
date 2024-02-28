#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024

int main() {
    int socket_fd;
    struct sockaddr_in server_address;

    // Create socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    char message[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_received;

    // Exchange messages until "Bye" is sent
    while (1) {
        printf("Enter your message (or type 'Bye' to exit): ");
        fgets(message, sizeof(message), stdin);

        // Remove newline character from input
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        // Send message to the server
        send(socket_fd, message, strlen(message), 0);

        if (strcmp(message, "Bye") == 0) {
            printf("Chat session terminated by client.\n");
            break;
        }

        // Receive response from the server
        bytes_received = recv(socket_fd, buffer, sizeof(buffer) , 0);
        buffer[bytes_received] = '\0';  // Null terminate the received data
        printf("Server: %s\n", buffer);
    }

    // Close socket
    close(socket_fd);

    return 0;
}


