#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>/*for getopt*/
#include <string.h>
#include <time.h>
#include <getopt.h>/*for getopt_long*/

#define	FMTSIZE			128
#define BUFSIZE			128
#define handle_error(msg)	{perror(msg);exit(EXIT_FAILURE);}

void print_usage(void)
{
	printf("Usage: [option]\n");
	printf("Option:\n");
	printf(" -y[2/4]    or	--year=[2/4]		The optput format of year\n");
	printf(" -m	    or	--month			The optput format of month\n");
	printf(" -d	    or	--day			The optput format of day\n");
	printf(" -H[12/24]  or	--hour=[12/24]		The optput format of hour\n");
	printf(" -M	    or	--minute		The optput format of minute\n");
	printf(" -S	    or	--second		The optput format of second\n\n");
	printf(" example:	./a.out -y4      -m -d -H 24      -M -S\n");
	printf(" 		./a.out --year=4 -m -d --hour 24 -M -S\n");
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

	static struct option long_options[] = {
		{"year",    1, NULL, 'y'},
		{"month",   0, NULL, 'm'},
		{"day",     0, NULL, 'd'},
		{"hour",    1, NULL, 'H'},
		{"minute",  0, NULL, 'M'},
		{"second",  0, NULL, 'S'},
		{NULL,      0, NULL,  0 }
	};

	/**************************************************************************************`	
	 *	ch = getopt_long(argc, argv, "y:mdH:MS", long_options, NULL);	
	 *		./a.out -y4 	  -m -d -H 24      -M -S
	 *		./a.out --year=4  -m -d --hour=24  -M -S 
	 *		./a.out --year 4  -m -d --hour 24  -M -S 
	 * 	each option must start with '-' or "--",otherwise it can't be parsed
	 * 	if the value of has_arg is greater than 1,formate must be --option=parameter  
	 **************************************************************************************/

	while((ch = getopt_long(argc, argv, "y:mdH:MS", long_options, NULL)) != -1)
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

