#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define RECVPORT		1999
#define	BUFSIZE			1024
#define	handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

int main(int argc, char *argv[])
{
	struct sockaddr_in seraddr;
	struct sockaddr_in cliaddr;
	char sendbuf[BUFSIZE] = {'\0'};
	char recvbuf[BUFSIZE] = {'\0'};
	char cliip[BUFSIZE] = {'\0'};;
	int cliaddr_len = 0;

	/* socket */
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd == -1)
		handle_error("socket()");
	printf("Socket succeed!\n");

	int on = 1;/* the value of 'on' should be greater than 0 */
	setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	bzero(&seraddr, sizeof(seraddr));/* use bzero make the empty of seraddr */
	seraddr.sin_family = AF_INET;
	inet_pton(AF_INET, "0.0.0.0", &seraddr.sin_addr);
	seraddr.sin_port = htons(RECVPORT);

	/* bind */
	if(bind(socketfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) == -1)
		handle_error("bind()");
	printf("Bind succeed!\n");

	/* listen */
	if(listen(socketfd, 5) == -1)
		handle_error("listen()");
	printf("Listen succeed!\n");

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr_len = sizeof(cliaddr);

	/* accept */
	int connfd = accept(socketfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
	if(connfd == -1)
		handle_error("accept()");
	printf("Accept succeed!\n");
	
	printf("Connect with ip[%s]\tport[%d]\n",
			inet_ntop(AF_INET, &cliaddr.sin_addr, cliip, BUFSIZE),
			ntohs(cliaddr.sin_port));

	while(1)
	{
		printf("[1]--------- Waiting for client data to arrive ---------\n");
		recv(connfd, recvbuf, BUFSIZE, 0);
		printf("server recvbuf = %s\n", recvbuf);
		
		printf("[2]--------- Plsase input data to send to client -------\n");
		scanf("%s", sendbuf);
		send(connfd, sendbuf, strlen(sendbuf)+1, 0);
	}

	/* close */
	close(socketfd);
	return 0;
}
