#include <stdio.h>

int main()
{
	int a= 0x12345678;
	char ch = (char)a;

	printf("ch = 0x%x\t%s\n", ch, (ch == 0x78)?"小端模式":"大端模式");

	return 0;
}
