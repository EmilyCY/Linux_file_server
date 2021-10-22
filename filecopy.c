#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

extern int errno;
void copy(char *fromfile, char *tofile);
int main(int argc, char *argv[])
{
  if (argc != 3)
    printf("usage : %s from to\n", argv[0]);
  else
    copy(argv[1], argv[2]);
  return 0;
} /* main */
void copy(char *fromfile,char *tofile)
{
  int infp, outfp;
  char buffer[1024], error_line[80];
  int charcount, writecount;
  infp = open(fromfile,O_RDONLY);
  if(infp == -1)
  {
    sprintf(error_line,"Error in opening file %s for reading ", fromfile);
  perror(error_line);
  exit(1);
  } /* if */
outfp = open(tofile,O_WRONLY|O_CREAT|O_EXCL,0644);
  if(outfp == -1)
  {
    sprintf(error_line,"Error in opening file %s for writing ", tofile);
    perror(error_line);
    close(infp);
    exit(1);
  } /* if */
  do
  {
    charcount = read(infp,buffer,sizeof(buffer));
    writecount = write(outfp,buffer,charcount);
  }while (charcount == sizeof(buffer));
 close(infp);
 close(outfp);
} /* copy */

