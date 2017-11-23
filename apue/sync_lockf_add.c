#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	PROCNUM			20
#define	BUFSIZE			1024
#define	FILENAME		"/tmp/out"
#define	handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

void add_function(void)
{
	int fd;
	char buf[BUFSIZE];
	FILE *stream = NULL;

	stream = fopen(FILENAME, "r+");
	if(stream == NULL)
		handle_error("fopen()");

	fd = fileno(stream);
	if(fd == -1)
		handle_error("fileno()");

	/* enter critical area */
	lockf(fd, F_LOCK, 0);
	
	fgets(buf, BUFSIZE, stream);
	fseek(stream, 0, SEEK_SET);
	fprintf(stream, "%d\n", atoi(buf)+1);
	fflush(stream);//\n为行缓冲模式，写文件为全缓冲模式，\n不是刷新缓冲区，所以应强制刷新缓冲区
	
	lockf(fd,F_ULOCK, 0);
	/* leave critical area */
	
	fclose(stream);
}


int main()
{
	int i;
	pid_t pid;

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

	return 0;
}
