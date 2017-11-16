#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
 *	ASCII Table
 *	A ~ F = 65 ~ 90
 *	    eg: 'D', ASCII('D')=68, ASCII('D')-ASCII('A')=3, hex('D')=13=3+10
 *	a ~ f = 97 ~ 122
 *	    eg: 'b', ASCII('b')=98, ASCII('b')-ASCII('a')=1, hex('b')=11=1+10
 *	0 ~ 9 = 48 ~ 57
 *	    eg: '8', ASCII('8')=56, ASCII('8')-ASCII('0')=8, hex('8')=8
 * */

long int hex_to_dec(long int num)
{
	char num_str[1024] = {'\0'};
	sprintf(num_str, "%x", num);/* amount to 'ltoa'*/

	int i, number;
	long int sum = 0;
	int len = strlen(num_str);

	for(i = 0; num_str[i]; i++)
	{
		if(num_str[i] <= 'f' && num_str[i] >= 'a')
			number = num_str[i]-97+10;
		else if(num_str[i] <= 'F' && num_str[i] >= 'A')
			number = num_str[i]-65+10;
		else
			number = num_str[i]-48;
		sum = sum + number*pow(16, --len);/* 'gcc *.c -lm' because use pow function */
	}

	return sum;
}

int main()
{
	long int hex = 0xabcdabcd;

	long int dec = 0;

	dec = hex_to_dec(hex);

	printf("hex = 0x%x\ndec = %ld\n", hex, dec);
	
	return 0;
}
