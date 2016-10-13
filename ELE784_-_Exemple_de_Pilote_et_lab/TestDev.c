/*
 ============================================================================
 Name        : TestDev.c
 Author      : Bruno
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
	   int 	fd;
	   char BufOut[10] = "BRUNO";
	   char BufIn[10];
	   int 	n, ret;

	   fd = open("/dev/pts/0", O_RDWR);

	   n = 0;
	   while (BufOut[n] != 0) {
		   ret = write(fd, &BufOut[n], 1);
		   if (ret > 0)
			   n += ret;
	   }
	   printf("write (%s:%u) => message = %s\n", __FUNCTION__, __LINE__, BufOut);

	   n = 0;
	   while (n < 9) {
		   ret = read(fd, &BufIn[n], 9-n);
		   if (ret > 0)
			   n += ret;
	   }
	   BufIn[9] = 0;
	   printf("read (%s:%u) => message = %s\n", __FUNCTION__, __LINE__, BufIn);

	   close(fd);

	   return EXIT_SUCCESS;
}
