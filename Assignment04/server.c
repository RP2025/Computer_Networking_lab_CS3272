#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ServerPort>\n", argv[0]);
        exit(1);
    }

    int serverPort = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    struct sockaddr_in serverAddr, clientAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        error("Error on binding");
    }

    socklen_t clientLen = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientLen);
        if (n < 0) {
            error("Error receiving data");
        }

        // Process received packet
        uint32_t sequenceNumber;
        uint8_t ttl;
        uint16_t payloadLength;

        memcpy(&sequenceNumber, buffer, sizeof(uint32_t));
        memcpy(&ttl, buffer + sizeof(uint32_t), sizeof(uint8_t));
        memcpy(&payloadLength, buffer + sizeof(uint32_t) + sizeof(uint8_t), sizeof(uint16_t));

        // Check sanity of the packet
        if (n != sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) + payloadLength) {
            // Malformed packet
            snprintf(buffer + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t), MAX_BUFFER_SIZE, "MALFORMED PACKET");
            sendto(sockfd, buffer, n, 0, (struct sockaddr *)&clientAddr, clientLen);
        } else {
            // Valid packet, decrement TTL and send back to client
            ttl--;
            memcpy(buffer + sizeof(uint32_t), &ttl, sizeof(uint8_t));
            sendto(sockfd, buffer, n, 0, (struct sockaddr *)&clientAddr, clientLen);
        }
    }

    close(sockfd);
    return 0;
}

