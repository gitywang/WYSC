#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define	RECVPORT		1999
#define	BUFSIZE			1024
#define handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

int main()
{
	struct sockaddr_in seraddr;
	struct sockaddr_in cliaddr;
	char recvbuf[BUFSIZE] = {'\0'};
	char sendbuf[BUFSIZE] = {'\0'};
	char ipstr[BUFSIZE] = {'\0'};	

	/* socket */
	int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd == -1)
		handle_error("socket()");
	printf("socket succeed!\n");

	/* bind */
	bzero(&seraddr, sizeof(seraddr));/* use bzero make the empty of seraddr */
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(RECVPORT);
	inet_pton(AF_INET, "0.0.0.0", &seraddr.sin_addr.s_addr);
	
	if(bind(socketfd, (struct sockaddr *)&seraddr, sizeof(seraddr)) < 0)
		handle_error("bind()");
	printf("bind succeed!\n");

	/* recvfrom */
	socklen_t cliaddr_len = sizeof(cliaddr);
	bzero(&cliaddr, sizeof(cliaddr));/* use bzero make the empty of cliaddr */
	while(1)
	{
		printf("[2]------- Waiting for client data to arrive  --------\n");
		if(recvfrom(socketfd, recvbuf, BUFSIZE, 0, (struct sockaddr *)&cliaddr, &cliaddr_len) < 0)
			handle_error("recvfrom()");

		inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, ipstr, BUFSIZE);
		
		printf("   message from %s\tport %d\n", ipstr, ntohs(cliaddr.sin_port));
		printf("   recvbuf = %s\n", recvbuf);
		
		printf("[1]------- Please input data to send to client --------\n");
		scanf("%s", sendbuf);
		if(sendto(socketfd, sendbuf, BUFSIZE, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) == -1)
			handle_error("sendto()");
	}

	/* close */
	close(socketfd);

	return 0;
}
