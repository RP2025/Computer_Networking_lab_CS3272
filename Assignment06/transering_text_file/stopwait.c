#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 69
#define MAX_DATA_SIZE 512

// TFTP opcode constants
#define RRQ 1   // Read request
#define WRQ 2   // Write request
#define DATA 3  // Data
#define ACK 4   // Acknowledgement
#define ERROR 5 // Error

// TFTP error codes
#define ERR_FILE_NOT_FOUND 1
#define ERR_ACCESS_VIOLATION 2
#define ERR_DISK_FULL 3
#define ERR_ILLEGAL_OPERATION 4
#define ERR_UNKNOWN_TID 5
#define ERR_FILE_EXISTS 6
#define ERR_NO_SUCH_USER 7

// Structure for TFTP packet

struct TFTP_Packet {
    unsigned short opcode;
    union {
        unsigned short block_num;
        char data[MAX_DATA_SIZE];
        char filename[MAX_DATA_SIZE];
        struct {
            unsigned short error_code; // Moved error_code here for error packets
            char error_msg[MAX_DATA_SIZE - sizeof(unsigned short)];
        };
    };
};




//tion to send a RRQ packet
void send_rrq_packet(int sockfd, struct sockaddr_in server_addr, char *filename) {
    struct TFTP_Packet rrq_packet;
    rrq_packet.opcode = htons(RRQ);
    strncpy(rrq_packet.filename, filename, MAX_DATA_SIZE);

    sendto(sockfd, &rrq_packet, sizeof(rrq_packet), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
}

// Function to receive a DATA packet
int recv_data_packet(int sockfd, struct sockaddr_in *server_addr, struct TFTP_Packet *data_packet) {
    socklen_t server_len = sizeof(*server_addr);
    int bytes_recv = recvfrom(sockfd, data_packet, sizeof(*data_packet), 0,
                              (struct sockaddr *)server_addr, &server_len);
    return bytes_recv;
}

// Function to send an ACK packet
void send_ack_packet(int sockfd, struct sockaddr_in server_addr, unsigned short block_num) {
    struct TFTP_Packet ack_packet;
    ack_packet.opcode = htons(ACK);
    ack_packet.block_num = htons(block_num);

    sendto(sockfd, &ack_packet, sizeof(ack_packet), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
}

// Function to handle errors
void handle_error(struct TFTP_Packet error_packet) {
    fprintf(stderr, "Error code %d: %s\n", ntohs(error_packet.error_code), error_packet.error_msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    char *filename = argv[2];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error in socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
    }

    // Send RRQ packet
    send_rrq_packet(sockfd, server_addr, filename);

    // Receive and process DATA packets
    unsigned short expected_block_num = 1;
    while (1) {
        struct TFTP_Packet data_packet;
        int bytes_recv = recv_data_packet(sockfd, &server_addr, &data_packet);
        if (bytes_recv < 0) {
            perror("Error in recvfrom");
            exit(EXIT_FAILURE);
        }

        if (data_packet.opcode == htons(DATA)) {
            if (ntohs(data_packet.block_num) == expected_block_num) {
                fwrite(data_packet.data, 1, bytes_recv - sizeof(data_packet.opcode) - sizeof(data_packet.block_num), stdout);
                send_ack_packet(sockfd, server_addr, expected_block_num);
                expected_block_num++;
            } else if (ntohs(data_packet.block_num) < expected_block_num) {
                // Resend ACK for duplicate packet
                send_ack_packet(sockfd, server_addr, ntohs(data_packet.block_num));
            }
        } else if (data_packet.opcode == htons(ERROR)) {
            handle_error(data_packet);
        }
    }

    close(sockfd);
    return 0;
}

