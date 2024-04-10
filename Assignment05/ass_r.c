#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>  // Added for inet_addr

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;

char interface[30];
char sender_ip_address[30];
char receiver_ip_address[30];
struct timespec timer;

unsigned char* buffer = NULL;

struct my_ethhdr {  // Renamed from ethhdr to my_ethhdr
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    __be16 h_proto;
};

void extract_ethernet_header(void *buffer) {
    struct my_ethhdr* eth = (struct my_ethhdr*)buffer;
    printf("Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
           (unsigned)eth->h_source[0], (unsigned)eth->h_source[1],
           (unsigned)eth->h_source[2], (unsigned)eth->h_source[3],
           (unsigned)eth->h_source[4], (unsigned)eth->h_source[5]);
    printf("Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
           (unsigned)eth->h_dest[0], (unsigned)eth->h_dest[1],
           (unsigned)eth->h_dest[2], (unsigned)eth->h_dest[3],
           (unsigned)eth->h_dest[4], (unsigned)eth->h_dest[5]);
    printf("Protocol : %d\n", ntohs(eth->h_proto));
}

void* timerFun(void* ptr) {
    nanosleep(&timer, NULL);
    pthread_cond_signal(&cond);
}

void* packet_processor(void* ptr) {
    pthread_mutex_lock(&locker);
    pthread_cond_wait(&cond, &locker);
    if (buffer == NULL) {
        printf("No ARP response received\nTIMED OUT\n");
        return NULL;
    }
    extract_ethernet_header(buffer);
    free(buffer);
    return NULL;
}
// Explaination

//pthread_mutex_lock(&locker);: This line acquires the mutex lock named locker, ensuring that only one thread can execute the critical section of code that follows. This prevents race conditions where multiple threads might try to access shared resources simultaneously.
//
//pthread_cond_wait(&cond, &locker);: This line waits on the condition variable cond while holding the mutex lock locker. The thread will block (i.e., pause its execution) until another thread signals the condition variable. This is typically done when some condition that the thread is waiting for becomes true. While waiting, the mutex lock is released, allowing other threads to proceed. When the condition variable is signaled, this function automatically re-acquires the mutex lock before continuing.
//
//if (buffer == NULL) { printf("No ARP response received\nTIMED OUT\n"); return NULL; }: Once the thread wakes up from waiting on the condition variable, it checks if the buffer variable is still NULL. If it is, it prints a message indicating that no ARP response was received within the timeout period and returns NULL, effectively terminating the thread's execution.
//
//extract_ethernet_header(buffer);: If an ARP response was received (i.e., buffer is not NULL), the thread proceeds to extract and process the Ethernet header from the received packet buffer using the extract_ethernet_header function.
//
//free(buffer);: After processing the received packet, the memory allocated for the packet buffer is released to prevent memory leaks.
//
//return NULL;: The function returns NULL to signify the end of its execution.

bool check_packet(unsigned char* packet) {
    unsigned int arp_protocol = 2054;
    struct my_ethhdr* eth = (struct my_ethhdr*)packet;
    if (ntohs(eth->h_proto) != arp_protocol) {
        memset(packet, 0, 1024);
        return false;
    }

    int pos = sizeof(struct my_ethhdr) + 6;
    int operation = 0;
    int y = packet[pos];
    operation = y;
    operation = operation << 8;
    pos++;
    y = packet[pos];
    operation = operation | y;
    if (operation != 2) {
        return false;
    }

    pos = sizeof(struct my_ethhdr) + 14;
    char received_ip[16] = {0};
    sprintf(received_ip, "%d.%d.%d.%d", packet[pos], packet[pos + 1], packet[pos + 2], packet[pos + 3]);
    if (strcmp(received_ip, receiver_ip_address) != 0) {
        return false;
    }
    return true;
}

// Explaination


//unsigned int arp_protocol = 2054;: This line initializes a variable arp_protocol with the value 2054, representing the ARP protocol type. ARP packets contain a protocol type field that indicates the type of the encapsulated packet.
//
//struct my_ethhdr* eth = (struct my_ethhdr*)packet;: This line casts the packet buffer to a pointer of type struct my_ethhdr, allowing access to the Ethernet header structure within the packet.
//
//if (ntohs(eth->h_proto) != arp_protocol) {: This condition checks if the protocol type of the received packet (after converting from network byte order to host byte order) matches the ARP protocol type. If they don't match, it indicates that the packet is not an ARP response packet, so the function clears the entire packet buffer using memset and returns false.
//
//int pos = sizeof(struct my_ethhdr) + 6;: This line initializes the variable pos to the position in the packet buffer where the ARP operation field begins. ARP packets contain an operation field that specifies whether the packet is a request or a response.
//
//The following lines read the ARP operation field from the packet buffer. It combines two bytes into a single integer to interpret the operation code.
//
//if (operation != 2) { return false; }: This condition checks if the operation code extracted from the packet buffer is not equal to 2, which indicates an ARP reply. If the operation code is not 2, it means the packet is not an ARP reply, so the function returns false.
//
//pos = sizeof(struct my_ethhdr) + 14;: This line updates the position pos to where the IP address of the sender in the ARP packet begins.
//
//char received_ip[16] = {0};: This line initializes a character array received_ip to store the IP address extracted from the packet buffer.
//
//sprintf(received_ip, "%d.%d.%d.%d", packet[pos], packet[pos + 1], packet[pos + 2], packet[pos + 3]);: This line extracts the IP address from the packet buffer and formats it as a string in the received_ip array.
//
//if (strcmp(received_ip, receiver_ip_address) != 0) { return false; }: This condition compares the extracted IP address with the receiver's IP address. If they don't match, it means the ARP response is not intended for the receiver, so the function returns false.
//
//If all the conditions are met, indicating that the packet is a valid ARP response for the receiver, the function returns true.



//int operation = 0;: This line initializes the variable operation to store the extracted ARP operation code.
//
//int y = packet[pos];: This line reads the first byte of the ARP operation field from the packet buffer at the position specified by pos and stores it in the variable y.
//
//operation = y;: This line assigns the value of y to the variable operation, effectively storing the first byte of the ARP operation code.
//
//operation = operation << 8;: This line shifts the value of operation left by 8 bits, effectively making room for the second byte of the ARP operation code. This operation effectively multiplies the first byte by 256, as each left shift by one bit doubles the value.
//
//pos++;: This line increments the position pos in the packet buffer to read the second byte of the ARP operation code.
//
//y = packet[pos];: This line reads the second byte of the ARP operation field from the packet buffer at the updated position specified by pos and stores it in the variable y.
//
//operation = operation | y;: This line performs a bitwise OR operation between the existing value of operation (which now contains the first byte shifted by 8 bits) and the value of y (which contains the second byte). This effectively combines the two bytes into a single integer representing the ARP operation code.
//
//if (operation != 2) { return false; }: This condition checks if the combined ARP operation code is not equal to 2. In the ARP protocol, an operation code of 2 corresponds to an ARP reply. If the operation code is not 2, it means the packet is not an ARP reply, so the function returns false.
//


void* waitForResponse(void* ptr) {
    int fd =*((int*)ptr);
    unsigned char packet[1024];
    memset(packet, 0, 1024);
    unsigned int arp_protocol = 2054;
    struct sockaddr sock_address;
    socklen_t size = sizeof(sock_address);

    while (1) {
        int recv_size = recvfrom(fd, packet, sizeof(packet), 0, &sock_address, &size);
        if (recv_size < 0) {
            perror("Receive failed");
            exit(1);
        }
        if (check_packet(packet) == false) continue;
        buffer = (unsigned char*)malloc(1024);
        memcpy(buffer, packet, 1024);
        break;
    }
    return NULL;
}

void fillARP(unsigned char* buffer, struct ifreq* ifreq_c) {
    int pos = sizeof(struct my_ethhdr);
    buffer[pos++] = 0;
    buffer[pos++] = 1;
    unsigned short opcode = 2048;
    buffer[pos++] = (opcode >> 8);
    buffer[pos++] = opcode;
    buffer[pos++] = 6;
    buffer[pos++] = 4;
    buffer[pos++] = 0;
    buffer[pos++] = 1;
    
    // source mac
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[0];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[1];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[2];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[3];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[4];
    buffer[pos++] = (ifreq_c->ifr_hwaddr).sa_data[5];
    
    
    in_addr_t ip = inet_addr(sender_ip_address);
    
    // source ip
    buffer[pos++] = ip;
    buffer[pos++] = (ip >> 8);
    buffer[pos++] = (ip >> 16);
    buffer[pos++] = (ip >> 24);
    
    // dest mac
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    buffer[pos++] = 0;
    
    //destination  ip
    ip = inet_addr(receiver_ip_address);
    buffer[pos++] = ip;
    buffer[pos++] = (ip >> 8);
    buffer[pos++] = (ip >> 16);
    buffer[pos++] = (ip >> 24);
}

int main(int argc, char* argv[]) {
    /* Command line argument handle */
    strcpy(interface, argv[1]);
    printf("%s\n", interface);
    strcpy(sender_ip_address, argv[2]);
    strcpy(receiver_ip_address, argv[3]);
    timer.tv_sec = atoll(argv[4]) / 1000000000;
    timer.tv_nsec = atoll(argv[4]) % 1000000000;
    
    // atoll -argument to long long
    
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("Socket failed to open :");
        exit(1);
    }

// jo packet system arp request krne pr bhejti thi wo hum khud bnakr bhejenge 
    unsigned char packet[64];
    memset(packet, 64, 0);
    struct sockaddr sock_address;
    socklen_t size = sizeof(sock_address);

    /* Index of the Interface */  // taken from document given by sir
    struct ifreq ifreq_i;
    memset(&ifreq_i, 0, sizeof(ifreq_i));
    strncpy(ifreq_i.ifr_name, interface, IFNAMSIZ - 1);  // Giving name of Interface
    if ((ioctl(fd, SIOCGIFINDEX, &ifreq_i)) < 0) {
        perror("Error in index \n");
        exit(1);
    }

    /* Mac Address of the Interface */
    struct ifreq ifreq_c;
    memset(&ifreq_c, 0, sizeof(ifreq_c));
    strncpy(ifreq_c.ifr_name, interface, IFNAMSIZ - 1);
    if ((ioctl(fd, SIOCGIFHWADDR, &ifreq_c)) < 0) {
        perror("Error in mac \n");
        exit(1);
    }

    struct my_ethhdr *eth = (struct my_ethhdr *)(packet);

    /* Filling Source mac address of the specified interface */
    memcpy(eth->h_source, ifreq_c.ifr_hwaddr.sa_data, ETH_ALEN);

    /* Filling destination mac address as Broadcast address 255.255.255.255.255.255 */
    memset(eth->h_dest, 255, ETH_ALEN);

    /* Filling the protocol Type... ARP */   // decimal value 2054
    unsigned short protocol_id = 2054;
    eth->h_proto = htons(protocol_id); // 0X0806 for ARP
    extract_ethernet_header((void*)packet);

    /* Filling ARP request packet */
    fillARP(packet, &ifreq_c);

    /* Sending ARP Packet to the Physical Layer */  // document given by Sir
    
    
    struct sockaddr_ll sadr_ll;   // socket addr link layer
    
    
    
    sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex;  // index of interface
    sadr_ll.sll_halen = ETH_ALEN;  // length of destination mac address
    memset(sadr_ll.sll_addr, 255, ETH_ALEN);
    
    
    int send_len = sendto(fd, packet, 64, 0, (const struct sockaddr*)&sadr_ll, sizeof(sadr_ll));
    if (send_len < 0) {
        perror("Sending Error ");
        exit(1);
    }

    pthread_t timer, receiver, processor;
    pthread_create(&processor, NULL, &packet_processor, NULL);
    
    
    pthread_create(&receiver, NULL, &waitForResponse, (void*)&fd);
    
    
    pthread_create(&timer, NULL, &timerFun, NULL);
    pthread_join(timer, NULL);
    pthread_cancel(receiver);
    close(fd);
    return 0;
}

// we have made packet for Arp request not response
