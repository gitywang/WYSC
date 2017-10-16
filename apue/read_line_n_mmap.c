#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

void read_line_n(char *addr, int line_number, off_t size)
{
	int step = 0;
	int enter = 0;
	char *print_start = NULL;

	if(line_number <= 0)
	{
		fprintf(stderr, "line number should equal or greater than 1\n");
		exit(EXIT_FAILURE);
	}
	else if(line_number == 1)
	{
		while(*addr != '\n')
		{
			printf("%c", *addr);
			addr++;
		}
		printf("\n");
	}
	else{
		while(1)
		{
			addr++;
			step++;
			if(step == size)
				break;
			if(*addr == '\n')
			{
				enter++;
				if(enter == line_number -1)
				{
					if(*(addr+1))
					{
						print_start = addr + 1;
						continue;
					}
				}
				if(enter == line_number)
					break;
			}
		}

		if(step == size)
			printf("the file have no line %d\n", line_number);
		if(print_start != NULL)
		{
			for(; *print_start != '\n'; print_start++)
				printf("%c", *print_start);
			printf("\n");
		}
	}
}


int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		fprintf(stderr, "usage: ./a.out <filename> <line number>\n");
		exit(EXIT_FAILURE);
	}

	int fd;
	char *addr;
	struct stat statbuf;

	fd = open(argv[1], O_RDONLY);
	if(fd < 0)
		handle_error("open()");

	if(fstat(fd, &statbuf) == -1)
		handle_error("fstat()");


	addr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, SEEK_SET);
	if(addr == MAP_FAILED)
		handle_error("mmap()");

	close(fd);

	read_line_n(addr, atoi(argv[2]), statbuf.st_size);

	munmap(addr, statbuf.st_size);

	return 0;
}
