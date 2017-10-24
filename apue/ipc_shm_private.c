#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define	MEMSIZE			1024
#define handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

int main()
{
	int shmid;
	pid_t pid;
	char *addr;

	shmid = shmget(IPC_PRIVATE, MEMSIZE, 0600);
	if(shmid ==  -1)
		handle_error("shmget()");

	pid = fork();
	if(pid < 0)
		handle_error("fork()");

	if(pid == 0) /* child write */
	{
		/* shmat */
		addr = shmat(shmid, NULL, 0);
		if(addr == (void *)-1)
			handle_error("child shmat()");
		strcpy(addr, "hello,world");
		/* shmdt */
		if(shmdt(addr) == -1)
			handle_error("child shmdt()");
		exit(EXIT_SUCCESS);
	}
	else /* father read */
	{
		wait(NULL);
		/* shmat */
		addr = shmat(shmid, NULL, 0);
		if(addr == (void *)-1)
			handle_error("father shmat()");
		printf("Shared memory data is : %s\n", addr);
		/* shmdt */
		if(shmdt(addr) == -1)
			handle_error("father shmdt()");
		exit(EXIT_SUCCESS);
	}

	/* shmctl */
	if(shmctl(shmid, IPC_RMID, 0) == -1)
		handle_error("shmctl()");


	return 0;
}
