#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>  //for sleep()
#define mailfile "/usr/mail/p8dx000"  //a file we monitor

int main()
{
  int oldsize, newsize;
  struct stat statbuff;
  stat(mailfile,&statbuff);
  oldsize = statbuff.st_size;
  newsize = oldsize;
  while (1)
  {
    sleep(10);
    stat(mailfile,&statbuff);
    newsize = statbuff.st_size;
    if (newsize > oldsize)
    {
      fprintf(stderr,"Mail has arrived\n");
      oldsize = newsize;
    } /* if */
  } /* while */
  return 0;
} /* main */

