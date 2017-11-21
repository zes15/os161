/*
 * meld.c
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

int
main(int argc, char *argv[])
{
	//static char writebuf[40] = "Twiddle dee dee, Twiddle dum dum.......\n";
	printf("\nRunning Meld tests...\n\n");
	static char test1[41];
	static char test2[41];

	const char *pn1;
	const char *pn2;
	const char *pn3;
	//int fd1, fd2, fd3, rv;
	int rv;

	(void)test1;
	(void)test2;
	(void)argc;
	(void)argv;

	pn1 = "testfile1.txt";
	pn2 = "testfile2.txt";
	pn3 = "testfile3.txt";

	rv = meld(pn1, pn2, pn3);

	if(rv != 0) {
		if(rv == EEXIST) err(rv, "%s - ", pn3);
		else printf("new error %d\n", rv);
		return rv;
	}
	//check all return values and throw appropriate error

	printf("Passed meld test.\n");
	return 0;
}

