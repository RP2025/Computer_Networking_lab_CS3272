#include<sys/types.h>
#include<stdio.h>
#include<sys/socket.h>
#include<error.h>
#include<stdlib.h>
#include<string.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include<bits/stdc++.h>
#include<fcntl.h>
char server_ip[15];
int PORT;


unsigned char* create_request_packet(char* file,char opcode,char *mode){
int packet_size = 2+strlen(file)+strlen(mode)+2;
unsigned char* buffer = (unsigned char*)malloc(packet_size);
memset(buffer,0,packet_size);
buffer[0]=0;
buffer[1] = opcode;
printf("Packet Size : %d\n",packet_size);
printf("File name size : %u\n", strlen(file));
printf("Mode name size : %u\n",strlen(mode));
strcpy((char*)&(buffer[2]),file);
strcpy((char*)&(buffer[2+strlen(file)+1]),mode);
buffer[packet_size-1] = 0;
return buffer;
}

unsigned char* create_ack_packet(unsigned short blockno){
unsigned char* buffer = (unsigned char*)malloc(4);
buffer[0] = 0;
buffer[1] = 4;
buffer[2] = (blockno>>8);
buffer[3] = blockno;
return buffer;
}


int main(int args, char* argv[]){
PORT = atoi(argv[2]);
int socket_fd  = socket(PF_INET,SOCK_DGRAM,0);
if(socket_fd < 0){
perror("Socket failed");
exit(1);
}
struct sockaddr_in server_address;
        memset(&server_address,0,sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(PORT);
        server_address.sin_addr.s_addr = inet_addr(argv[1]);

unsigned char* buffer = create_request_packet(argv[3],1,argv[4]);
printf("File :  %s\n",(char*)&buffer[2]);
printf("Mode : %s\n", (char*)&buffer[2+strlen(argv[3])+1]);
int packet_size  = 4+strlen(argv[3])+strlen(argv[4]);
int send_size = sendto(socket_fd,(void*)buffer,packet_size,0,(struct sockaddr*)&server_address,sizeof(server_address));
free(buffer);
if(send_size < 0){
perror("Request failed");
exit(1);
}

socklen_t size;

int file_fd = open(argv[5], O_RDWR | O_CREAT, 0666);

while(true){
char buffer[1024];
int receive_size = recvfrom(socket_fd,(void*)buffer,1024,0,(struct sockaddr*)&server_address, &size);
if(receive_size <= 0){
printf("Error\n");
break;
}
printf( "opcode = %d\n", (int) buffer[1]);
write(file_fd,(void*)&buffer[4],receive_size-4);
unsigned short block_no = (unsigned short)buffer[2];
block_no = block_no<<8;
block_no = block_no | (unsigned short)buffer[3];
printf("block_no : %d\n", int(block_no));
unsigned char *ack = create_ack_packet(block_no);
ack[2] = buffer[2];
ack[3] = buffer[3];
sendto(socket_fd,(void*)ack,4,0,(struct sockaddr*)&server_address,sizeof(server_address));
if(receive_size < 516)break;
}
close(socket_fd);
close(file_fd);
return 0;

}
