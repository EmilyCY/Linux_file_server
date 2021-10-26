#define _XOPEN_SOURCE 200 // feature test macro
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int main()
{
    struct dirent *dp;
    DIR *directory;
    struct stat buffer;
    char path[2046];

    if( (directory = opendir("/home/kali/Documents/OSM")) == NULL)
    printf("open directory failed");

    while ((dp = readdir(directory)) != NULL)
    {
        sprintf(path, "/home/kali/Documents/OSM/%s", dp->d_name);
        if (stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode)) // ignore subfolders
        {
            //if (strcmp(dp->d_name, ".") != 0) // ignore hidden files and file names with ..
            //{
                //namelen = namelen + strlen(dp->d_name);
                printf("%s, %d\n", dp->d_name, strlen(dp->d_name)); // print out all files in the directory
            //}
        }
    }

    closedir(directory);
}
