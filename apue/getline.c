#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	FILE *stream = NULL;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	if(argc < 2)
	{
		printf("Usage: ./a.out <filename>\n");
		exit(EXIT_FAILURE);
	}

	stream = fopen(argv[1], "r");
	if(stream == NULL)
	{
		perror("fopen()");
		exit(EXIT_FAILURE);
	}

	while((read =getline(&line, &len, stream)) != -1)
	{
		printf("Retrieved line of length %zu :\n", read);
		printf("%s", line);
		sleep(1);
	}

	free(line);
	fclose(stream);

	exit(EXIT_SUCCESS);
}
