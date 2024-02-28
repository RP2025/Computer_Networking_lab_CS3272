#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

uint64_t getCurrentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <P> <TTL> <NumPackets>\n", argv[0]);
        exit(1);
    }

    const char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);
    int payloadSize = atoi(argv[3]);
    int ttl = atoi(argv[4]);
    int numPackets = atoi(argv[5]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        error("Error converting IP address");
    }

    char buffer[MAX_BUFFER_SIZE];

    for (int i = 0; i < numPackets; ++i) {
        // Prepare packet
        
	uint32_t sequenceNumber = i;
        uint8_t ttlValue = ttl;
        uint16_t payloadLength = payloadSize;

        memcpy(buffer, &sequenceNumber, sizeof(uint32_t));
        memcpy(buffer + sizeof(uint32_t), &ttlValue, sizeof(uint8_t));
        memcpy(buffer + sizeof(uint32_t) + sizeof(uint8_t), &payloadLength, sizeof(uint16_t));

        // Send packet to server
        ssize_t sentBytes = sendto(sockfd, buffer, sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t) + payloadSize, 0,
                                   (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            error("Error sending data");
        }

        // Receive the packet back from the server
        memset(buffer, 0, sizeof(buffer));
        socklen_t serverLen = sizeof(serverAddr);
        ssize_t recvBytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddr, &serverLen);
        if (recvBytes < 0) {
            error("Error receiving data");
        }

	char msg[1024];
	for(int i=7;i<recvBytes;i++)
	{
		msg[i-7]=buffer[i];
	}
		printf("%s",msg);

        // Process received packet and calculate Round Trip Time (RTT)
        uint64_t currentTime = getCurrentTimeMillis();
        uint32_t receivedSequenceNumber;
        memcpy(&receivedSequenceNumber, buffer, sizeof(uint32_t));

        // Check if the packet is malformed
        if (recvBytes > sizeof(uint32_t) && strncmp(buffer + sizeof(uint32_t), "MALFORMED PACKET", sizeof("MALFORMED PACKET") - 1) == 0) {
            printf("Received malformed packet for sequence number %u\n", receivedSequenceNumber);
        } else {
            uint64_t rtt = getCurrentTimeMillis() - currentTime;
            printf("Round Trip Time for sequence number %u: %lu ms\n", receivedSequenceNumber, rtt);
        }
    }

    close(sockfd);
    return 0;
}

