#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>  //for exit()

int main(int argc, char *argv[])
{
  DIR *directory;
  struct dirent *dp;
  if (argc<2)
  {
    printf("Usage %s directory\n", argv[0]);
    exit(1);
  }
  else
  {
    directory = opendir(argv[1]);
    while ((dp = readdir(directory)) != NULL)
    printf("%s\n",dp->d_name);
  }
} /* main */

