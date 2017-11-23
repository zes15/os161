/*
 * ZACH SIROTTO - meld.c UNIT TEST
 *
 * NOTE: this test case will meld test1.txt and test2.txt
 * 	 into test3.txt AS LONG AS test3.txt DOES NOT ALREADY EXIST.
 *	 This is because, in the write up, Dr. Yu specifies that
 *	 if the third file exists, return with an error.
 *
 *	 This means that if you want to re-run this test, you must
 *	 delete whatever pn3 is set to, which in this case: is test3.txt
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	
	int rv;
	int fd[3];

	/* readbuf for displaying that meld worked successfully */
	static char readbuf[25];
	
	/* declare the content of our two input files */
	static char test1[13] = "AAAABBBBCCCC\0";
	static char test2[13] = "ddddeeeeffff\0";
	
	const char *pn1;
	const char *pn2;
	const char *pn3;

	pn1 = "test1.txt";
	pn2 = "test2.txt";
	pn3 = "test3.txt";

	printf("\nRunning Meld unit test...\n\n");

	/* create first input file */
	printf("Creating %s containing: %s\n", pn1, test1);
	/* open file for writing and create it if necessary */
	fd[0] = open(pn1, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	/* check for errors */
	if (fd[0] < 0) { err(1, "%s: open for writing test data", pn1); }
	/* write our test data to input file 1 */
	rv = write(fd[0], test1, 12);
	/* check for errors */
	if(rv<0) { err(1, "%s: writing to input file 1", pn1); }
	/* close test file 1 */
	rv = close(fd[0]);
	/* check for errors */
	if(rv<0) { err(1, "%s: close input file 1", pn1); }
	
	printf("Successfully created %s\n\n", pn1);
	
	/* create second input file */
	printf("Creating %s containing: %s\n", pn2, test2);
	/* open file for writing and create it if necessary */
	fd[1] = open(pn2, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	/* check for errors */
	if (fd[1] < 0) { err(1, "%s: open for writing test data", pn2); }
	/* write our test data to input file 1 */
	rv = write(fd[1], test2, 12);
	/* check for errors */
	if(rv<0) { err(1, "%s: writing to input file 2", pn2); }
	/* close test file 1 */
	rv = close(fd[1]);
	/* check for errors */
	if(rv<0) { err(1, "%s: close input file 2", pn2); }
	
	printf("Successfully created %s\n\n", pn2);

	/* now that we have our test files, call meld */
	printf("Calling meld...\n");
	rv = meld(pn1, pn2, pn3);

	/* check meld for errors and return the error */
	if(rv != 0) {
		err(rv, "");
		return rv;
	}

	printf("Successfully melded %s and %s into %s.\n", pn1, pn2, pn3);

	/* read contents of pn3 to show that meld worked successfully */
	fd[2] = open(pn3, O_RDONLY);
	if(fd[2]<0) { err(1, "%s: opening after meld", pn3); }
	rv = read(fd[2], readbuf, 24);
	readbuf[24] = '\0'; // add null char at end for printing
	rv = close(fd[2]);
	if(rv<0){ err(1, "%s: closing after meld", pn3); }
	
	/* display melded contents */
	printf("\n%s now contains: %s\n", pn3, readbuf);
	
	/* set return value for success */
	return 0;
}

