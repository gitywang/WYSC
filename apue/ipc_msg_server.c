#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define	handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}
#define	KEYPATH			"/etc/services"
#define	KEYPROJ			'x'
#define	MSGSIZE			1024

struct msg_st
{
	long mtype;
	char message[MSGSIZE];
};

int main()
{
	int msgid;
	key_t key;
	struct msg_st *recvbuf;

	/* ftok */
	key = ftok(KEYPATH, KEYPROJ);
	if(key == -1)
		handle_error("ftok()");

	/* msgget, server create msssage queue */
	msgid = msgget(key, IPC_CREAT|0600);
	if(msgid == -1)
		handle_error("msgget()");

	recvbuf = (struct msg_st *)malloc(sizeof(struct msg_st));
	if(recvbuf == NULL)
		handle_error("malloc");

	/* msgrcv */
	if(msgrcv(msgid, recvbuf, sizeof(struct msg_st), 0, 0) == -1)
		handle_error("msgrcv()");

	printf("Received message queue data is : %s\n", recvbuf->message);

	/* msgctl, server destroy message queue */
	if(msgctl(msgid, IPC_RMID, 0) == -1)
		handle_error("msgctl()");

	free(recvbuf);
	return 0;
}
