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



using namespace std;

int PORT;

char ip_address[20];

char ttl;

unsigned short payload_len;

int num_packet;



double total =0;



void func(int socket_fd, unsigned int pay){

        struct sockaddr_in server_address;

        memset(&server_address,0,sizeof(server_address));

        server_address.sin_family = AF_INET;

        server_address.sin_port = htons(PORT);

        server_address.sin_addr.s_addr = inet_addr(ip_address);

                unsigned char buffer[7+pay];
                buffer[0]=0;
                buffer[1]=0;
                buffer[2]=0;
                buffer[3]=0;
                buffer[4]=ttl;
                unsigned int x = pay;
                buffer[6] = x;
                x = x>>8;
                buffer[5]=x;

                for(int i=0;i<pay;i++){
                        char c = rand();
                        buffer[7+i]=c;
                }

        clock_t t;

	for(int i=0;i<50;i++){

		buffer[4] = ttl;
		buffer[3] = i;
		
		socklen_t size = sizeof(server_address);

		t = clock();

		while(true){
        		sendto(socket_fd,(void*)buffer, sizeof(buffer),0,(struct sockaddr*)&server_address, sizeof(server_address));

			int recv_size = recvfrom(socket_fd,(void*)buffer, sizeof(buffer),0, (struct sockaddr*)&server_address, &size);

			buffer[4]--;

			if(buffer[4] == 0)break;
		}

		t = clock() - t;
		double time_taken = ((double)t)/CLOCKS_PER_SEC;
		total += time_taken;

	}

	/*
                char message[1024];
                for(int i=0;i<recv_size-7;i++){
                        message[i]= response[7+i];
                }
                message[recv_size-7]= '\0';

                printf("The receive from the server is %s",message);
		printf("%d\n",((int)response[4]));

	*/

}



int main(int argc, char* argv[]){

        int socket_fd  = socket(PF_INET,SOCK_DGRAM,0);



        PORT = stoi(argv[2]);

        strcpy(ip_address,argv[1]);

        ttl = stoi(argv[3]);

        if(socket_fd == -1){

                perror("Socket Error");

                _exit(1);

        }



        for(int i=0;i<100;i++){
		func(socket_fd, 100+i*10);
		printf("Packet size : %d  && time : %f \n",100+i*10,total);
		total =0;
	}

        close(socket_fd);

}
