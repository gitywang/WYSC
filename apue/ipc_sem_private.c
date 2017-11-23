#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#define	PROCNUM			20
#define	BUFSIZE			1024
#define	FILENAME		"/tmp/out"
#define	handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

static int semid;

static void P()
{
	struct sembuf op;

	op.sem_num = 0;/* semnum = 0 ~ nsems-1 */
	op.sem_op = -1;
	op.sem_flg = 0;

	while(semop(semid, &op, 1) == -1)
	{
		if(errno == EINTR  || errno == EAGAIN)
			continue;
		else
			handle_error("semop()->P");
	}
}

static void V()
{
	struct sembuf op;

	op.sem_num = 0;/* semnum = 0 ~ nsems-1 */
	op.sem_op = 1;
	op.sem_flg = 0;

	while(semop(semid, &op, 1) == -1)
	{
		if(errno == EINTR || errno == EAGAIN)
			continue;
		else
			handle_error("semop()->V");
	}
}

void add_function(void)
{
	int fd;
	char buf[BUFSIZE];
	FILE *stream = NULL;

	stream = fopen(FILENAME, "r+");
	if(stream == NULL)
		handle_error("fopen()");

	/* fp --> fd */
	fd = fileno(stream);
	if(fd == -1)
		handle_error("fileno()");

	/* enter critical area */
	//lockf(fd, F_LOCK, 0);
	P();

	fgets(buf, BUFSIZE, stream);
	fseek(stream, 0, SEEK_SET);
	fprintf(stream, "%d\n", atoi(buf)+1);
	fflush(stream);//\n为行缓冲模式，写文件为全缓冲模式，\n不是刷新缓冲区，所以应强制刷新缓冲区
	
	//lockf(fd,F_ULOCK, 0);
	V();
	/* leave critical area */
	
	fclose(stream);
}


int main()
{
	int i;
	pid_t pid;

	/* semget */
	semid = semget(IPC_PRIVATE, 1, 0600);//nsems=1
	if(semid == -1)
		handle_error("semget()");

	/* semctl */
	if(semctl(semid, 0, SETVAL, 1) == -1)//对下标为0的成员赋值为1
		handle_error("semctl()");

	for(i = 0; i < PROCNUM; i++)
	{
		pid = fork();
		if(pid < 0)
			handle_error("fork()");

		if(pid == 0)
		{
			add_function();
			exit(EXIT_SUCCESS);
		}
	}


	for(i = 0; i < PROCNUM; i++)
		wait(NULL);

	/* semctl */
	if(semctl(semid, 0, IPC_RMID) == -1)
		handle_error("semctl()");

	return 0;
}
