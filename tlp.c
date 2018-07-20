#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* usleep(3) */

/* 
 * A sort of software teleprompter
 * Orestes Leal Rodriguez, 2015 <olealrd1981@gmail.com
 */

int main(int argc, char *argv[])
{
   FILE *fd;
   char cChar, *pcChar = &cChar;

   if (argc < 2) {
   	printf("%s input-file\n", argv[0]);
   	return EXIT_SUCCESS;
   }

   fd = fopen(argv[1], "r");
   if (fd == NULL) {
   	printf("Error: couln't open file %s", argv[1]);
   	return EXIT_FAILURE;
   }
   
   for (; fread(pcChar, 1, 1, fd); fflush(stdout)) {
   	printf("%c", cChar);
   	usleep(20000);
   }
   	
   fclose(fd);

   return EXIT_SUCCESS;
}

