#include <stdio.h>
#include <setjmp.h>

/* 	setjmp.h
 * 	提供了一种避免通常的函数调用和返回顺序的途径
 * 	它允许立即从一个多层嵌套的函数调用中返回
 * */

jmp_buf buf;

void c()
{
	printf("%s() -- Before calling longjmp()\n", __FUNCTION__);
	longjmp(buf, 1);
	printf("%s() -- After  calling longjmp()\n", __FUNCTION__);
}

void b()
{
	printf("%s() -- Before calling c()\n", __FUNCTION__);
	c();
	printf("%s() -- After  calling c()\n", __FUNCTION__);
}

void a()
{
	printf("%s() -- Before calling b()\n", __FUNCTION__);
	b();
	printf("%s() -- After  calling b()\n", __FUNCTION__);
}


int main()
{
	int ret = -1;

	/*	若直接调用setjmp()，那么返回值为0
	 *	若由于调用longjmp()而调用setjmp(),那么返回值非0
	 * */
	
	ret = setjmp(buf);
	if(ret == 0)
	{
		printf("%s() -- first calling setjmp()\n", __FUNCTION__);
		a();
	}
	else
	{
		printf("%s() -- second calling setjmp()\n", __FUNCTION__);
		printf("%s() -- jump back here with value %d\n", __FUNCTION__, ret);	
	}


	return 0;
}
