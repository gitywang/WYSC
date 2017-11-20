#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>/*for getopt*/
#include <string.h>
#include <time.h>

#define	FMTSIZE			128
#define BUFSIZE			128
#define handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

void print_usage(void)
{
	printf("Usage: [option]\n");
	printf("Option:\n");
	printf(" -y[2/4]	The optput format of year\n");
	printf(" -m		The optput format of month\n");
	printf(" -d		The optput format of day\n");
	printf(" -H[12/24]	The optput format of hour\n");
	printf(" -M		The optput format of minute\n");
	printf(" -S		The optput format of second\n\n");
	printf(" example:	./a.out -y 4 -m -d -H24 -M -S\n");
	printf(" output:	2017-11-20  15:10:26\n\n");
	exit(EXIT_FAILURE);;
}

int main(int argc, char *argv[])
{
	time_t timep;
	struct tm *tm;
	char ch;
	char timeformat[FMTSIZE] = {'\0'};
	char timebuf[BUFSIZE] = {'\0'};

	if(argc < 2)
		print_usage();

	memset(timeformat, '\0', FMTSIZE);
	memset(timebuf, '\0', BUFSIZE);

	time(&timep);
	tm = localtime(&timep);
	printf("%d-%d-%d  %d:%d:%d\n\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	/**************************************************************************************`	
	 *	ch = getopt(argc, argv, "y:mdH:MS");	
	 *		./a.out -y4 -m -d -H 24 -M -S
	 *	ch = getopt(argc, argv, "y::mdH::MS");
	 *		./a.out -y4  -m -d -H24  -M -S 
	 * 	each option must start with '-',otherwise it can't be parsed
	 * 	if there are two ':' behind option,then the parameter must follow the option 
	 **************************************************************************************/

	while((ch = getopt(argc, argv, "y:mdH:MS")) != -1)
	{
		switch(ch)
		{
			case 'y':
				/* every time getopt is called,'optarg' automatically indexes the next element of 'argv' */
				if(!strcmp(optarg, "2"))
					strncat(timeformat, "%y-", FMTSIZE);/* for strfime */
				else if(!strcmp(optarg, "4"))
					strncat(timeformat, "%Y-", FMTSIZE);
				else
					print_usage();
				break;
			case 'm':
				strncat(timeformat, "%m-", FMTSIZE);
				break;
			case 'd':
				strncat(timeformat, "%d  ", FMTSIZE);
				break;
			case 'H':
				if(!strcmp(optarg, "12"))
					strncat(timeformat, "%I:", FMTSIZE);
				else if(!strcmp(optarg, "24"))
					strncat(timeformat, "%H:", FMTSIZE);
				else
					print_usage();
				break;
			case 'M':
				strncat(timeformat, "%M:", FMTSIZE);
				break;
			case 'S':
				strncat(timeformat, "%S", FMTSIZE);
				break;
			default:
				print_usage();
				break;
		}
	}

	strftime(timebuf, BUFSIZE, timeformat, tm);
	puts(timebuf);

	return 0;
}

