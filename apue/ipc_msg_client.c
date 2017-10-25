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
#define	MSG_STU			1

struct msg_st
{
	long mtype;
	char message[MSGSIZE];
};

int main()
{
	int msgid;
	key_t key;
	struct msg_st *sendbuf = NULL;

	/* ftok */
	key = ftok(KEYPATH, KEYPROJ);
	if(key == -1)
		handle_error("ftok()");

	/* msgget, client don't need create again */
	msgid = msgget(key, 0);
	if(msgid == -1)
		handle_error("msgget()");

	sendbuf = (struct msg_st *)malloc(sizeof(struct msg_st));
	if(sendbuf == NULL)
		handle_error("malloc()");

	sendbuf->mtype = MSG_STU;
	strcpy(sendbuf->message, "hello,world!");

	/* msgsnd, sendbuf size should subtract sizeof(long) */
	if(msgsnd(msgid, sendbuf, sizeof(struct msg_st)-sizeof(long), 0) == -1)
		handle_error("msgsend()");

	printf("Send message queue data OK\n");

	free(sendbuf);
	return 0;
}
