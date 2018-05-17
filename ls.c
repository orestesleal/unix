#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* 
 *  Minimal implementation UNIX's ls command for listing
 *  directories and files
 *
 *  this version don't list files directly by name, only directories,
 *  and thus files in it, also there is not sorting, etc.
 *
 *  Available switches: none, super simple listing
 *                      -l for a column list
 *                      -a for listing everything including . and ..,
 *                         it requires -l
 *
 *   examples
 *             ./ls        # list the current directory
 *             ./ls /tmp
 *             ./ls /etc -l
 *             ./ls $HOME -la 
 *
 *  Orestes Leal Rodriguez, 2016, <olealrd1981@gmail.com>
 */ 

int main(int argc, char *argv[])
{
   DIR *d;
   struct dirent *dir_ptr;  /*  pointer to a directory entry after readdir  */
   char sep = ' '; 

   if (argc < 2) 
       d = opendir(".");
   else
       d = opendir(argv[1]);

   if (d == NULL) {
      perror("opendir: ");
      exit (EXIT_FAILURE);
   }

   if (argc > 2 && !strncmp(argv[2], "-l", 2))
      sep = '\n';

   errno = 0;  /* reset errno before the call to readir(3) */

   while ((dir_ptr = readdir(d)) != NULL) {

      if (dir_ptr->d_name[0] != '.' || strncmp(dir_ptr->d_name, "..", 2) > 0) {
         printf("%s%c", dir_ptr->d_name, sep);
      }
      else if (argc > 2) {
         if (argv[2][2] == 'a')
            printf("%s%c", dir_ptr->d_name, sep);
      }
   }

   if (errno != 0) {  /* readir failed */
      perror("readir: ");
      exit (EXIT_FAILURE);
   }

   closedir(d);

   if (sep == ' ') printf("\n");

   return EXIT_SUCCESS;
}
