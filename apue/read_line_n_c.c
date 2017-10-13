#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void read_line_n_c(FILE *stream, int line_number)
{
	char ch;
	int enter_count = 0;
	int flag = 0;

	if(line_number == 0)
	{
		printf("line number should equal or greater than 1\n");
		exit(EXIT_FAILURE);
	}
	else if(line_number == 1)
	{
		while((ch = fgetc(stream)) != '\n')
			putchar(ch);
		putchar('\n');
	}
	else
	{
		while((ch = fgetc(stream)) != EOF)
		{
			if(ch == '\n')
			{
				enter_count++;
				if(enter_count == line_number -1)
				{
					flag = 1;
					continue;
				}
				if(enter_count == line_number)
				{
					flag = 2;
					break;
				}
			}
			if(flag == 1)
				putchar(ch);
		}
	}
	if(flag != 2)
		fprintf(stderr, "file have no line %d\n", line_number);

	putchar('\n');

	fclose(stream);
}

int main(int argc, char *argv[])
{
	FILE *stream = NULL;


	if(argc < 3)
	{
		fprintf(stderr, "usage: ./a.out <filename> <line number>\n");
		exit(EXIT_FAILURE);
	}

	stream= fopen(argv[1], "r");
	if(stream == NULL)
	{
		perror("fopen()");
		exit(EXIT_FAILURE);
	}

	read_line_n_c(stream, atoi(argv[2]));

	return 0;
}
