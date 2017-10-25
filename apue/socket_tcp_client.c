#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define RECVPORT		1999
#define	BUFSIZE			1024
#define handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

int main(int argc, char *argv[])
{
	struct sockaddr_in seraddr;
	char sendbuf[BUFSIZE] = {'\0'};
	char recvbuf[BUFSIZE] = {'\0'};

	if(argc < 2)
	{
		fprintf(stderr, "Usge: ./client <ip>");
		exit(EXIT_FAILURE);
	}

	const char *ip = argv[1];

	/* socket */
	int socketfd = socket(AF_INET , SOCK_STREAM, 0);
	if(socketfd == -1)
		handle_error("socket()");
	printf("Socket succeed!\n");

	bzero(&seraddr, sizeof(seraddr));/* use bzero make the empty of seraddr */
	seraddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip ,&seraddr.sin_addr.s_addr);
	seraddr.sin_port = htons(RECVPORT);

	/* connect */
	if(connect(socketfd, (struct sockaddr *)&seraddr, sizeof(seraddr)) == -1)
		handle_error("connect()");
	printf("Connect succeed!\n");
		
	while(1)
	{
		printf("[1]--------- Please input data to send to server -------\n");
		scanf("%s", sendbuf);
		send(socketfd, sendbuf, strlen(sendbuf)+1 , 0);
		
		printf("[2]--------- Waiting for client data to arrvie ---------\n");
		recv(socketfd, recvbuf, sizeof(recvbuf), 0);
		printf("client recvbuf = %s\n", recvbuf);
	}

	/* close */
	close(socketfd);
	return 0;
}


