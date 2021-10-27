/*ICT60001_A4_103329805.c 
This program is a simple file server running on linux that transfer files 
from the directory: /root/server/ to the client directory: /root/client/

Author: Emily Cheng
ID: 103329805
Last updated: 27/10/2021
*/

#define _XOPEN_SOURCE 500 // feature test macro
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
#include <fcntl.h>
#include <errno.h>

/*other macros*/
#define SIZE sizeof(struct sockaddr_in)
#define HOST "kali"
#define PORT 1337
#define PATH_MAX 4096
#define FILE_NAME_MAX 255

/*function prototypes*/
void client(char *clientdir);
void server(char *serverdir);
void catcher(int sig);
void sendDirList(char *dirpath, int socket);
int sendAll(int socket, void *buffer, size_t length, int flags);
void listFileNames(int socket);
void selectFile(int socket, char *filename);
void sendFileToClient(int socketc);
void readFileOnServer(char *filename, int socketc);
int checkFileSize(char *filepath);
void receiveFile(int socketc, char *clientdir, char *filename);

int newsockfd, sockfd, i;
char c, rc;
int pid;

static struct sigaction act;
struct sockaddr_in serverc = {AF_INET, PORT}; //family, port in byte order, interbet address
struct sockaddr_in servers = {AF_INET, PORT, INADDR_ANY};
struct hostent *hp; // h_name: official_name, h_addr: first host address, h_length: length of the address

/*main function for managing command-line options (client or server)
@param argc integer representing number of command line arguments including program name
@param argv Char array containing command line arguments*/
int main(int argc, char *argv[])
{
	char address[20];
	char *iaddress = &address[0]; // to save the host address
	int newport;
	char serverdir[PATH_MAX] = "/root/server/";
	char clientdir[PATH_MAX] = "/root/client/";

	/*find the ip address of host just in case it changes)*/
	hp = gethostbyname(HOST);
	memmove(hp->h_addr_list[0], &serverc.sin_addr, hp->h_length); // copy host address to IP socket address
	strcpy(iaddress, (char *)inet_ntoa(serverc.sin_addr));		  // convert from binary to string form and save to the memory

	if (argc == 4)
	{											  /*sscanf returns number of variables filled */
		if (sscanf(argv[3], "%d", &newport) == 1) /*This option allows pairs of users to set up a private*/
		{										  /* conversation by specifying a different port number. */
			serverc.sin_port = newport;
			servers.sin_port = newport;
			printf("Using port number %d\n", servers.sin_port);
		}
		else
		{
			printf("Using default port number %d\n", servers.sin_port);
		}
	}

	if (argc == 3 || argc == 4)	   /*This option allows the program to be used on a server other than default.*/
	{							   /*The gethostname() is not used as this is unreliable in linux and tends to*/
								   /*return a truncated version of the host address. */
		strcpy(iaddress, argv[2]); // 3rd arg is the specified server address
		serverc.sin_addr.s_addr = inet_addr(iaddress);
		printf("Using server at %s\n", iaddress);
	}
	else
		printf("Using server at %s\n", iaddress);

	if (argc >= 2 && argc <= 4) // to find out the choice of a server or on client
	{
		if (strcmp(argv[1], "server") == 0)
			server(serverdir);
		else
		{
			client(clientdir);
		}
		exit(0);
	}

	/*else invalid input*/

	printf("usage: %s server|client [ip_address [port]]\n", argv[0]);
	printf("where port is a port number (default port: %d).\n", PORT);
	printf("      ip_address is an address in nnn.nnn.nnn.nnn format\n");
	return 0;
}

/*Act as a server and wait for and receive connections*/
void server(char *serverdir)
{
	char selectedfile[FILE_NAME_MAX + 1];
	printf("Running as server. \t ^C to hangup. \n");

	act.sa_handler = catcher;
	sigfillset(&(act.sa_mask));
	sigaction(SIGINT, &act, NULL); /*catch ^C interupts*/

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Socket failed: ");
		exit(0);
	}

	if (bind(sockfd, (struct sockaddr *)&servers, SIZE) == -1)
	{
		perror("Bind failed: ");
		exit(0);
	}
	if (listen(sockfd, 5) == -1)
	{
		perror("Listen failed: ");
		exit(0);
	}

	while (1) /*accept connections*/
	{
		//NULL for sockaddr means that client IP is discarded.
		if ((newsockfd = accept(sockfd, NULL, NULL)) == -1)
		{
			perror("Accept error: ");
			continue;
		}

		printf("Connected\n");

		sendDirList(serverdir, newsockfd);
		sendFileToClient(newsockfd);
	}
}

/*Signal handler used by the client function
@param sig The signal as defined in signal.h*/
void catcher(int sig)
{
	printf("Interupt caught\n");
	close(newsockfd);
	exit(0);
}

/*Make a call to the server
@param iaddress A char* containing the IP address of the server*/
void client(char *clientdir)
{
	char selectedfile[FILE_NAME_MAX + 1];

	printf("Running as client. \t ^C to hangup. \n");

	act.sa_handler = catcher;
	sigfillset(&(act.sa_mask));
	sigaction(SIGINT, &act, NULL); /*catch ^C interupts*/

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Socket failed: ");
		exit(0);
	}

	if (connect(sockfd, (struct sockaddr *)&serverc, SIZE) == -1)
	{
		perror("Connect failed: ");
		exit(0);
	}

	listFileNames(sockfd);
	selectFile(sockfd, selectedfile);
	receiveFile(sockfd, clientdir, selectedfile);
}

/* This function sends a list of regular files in the specified directory, ignoring subfolders*/
void sendDirList(char *dirpath, int socket)
{
	DIR *directory;
	struct dirent *dp;
	char path[PATH_MAX];
	struct stat buffer;
	int count = 0;
	int sentcount = 0;
	char message[30];
	ssize_t sent;

	if ((directory = opendir(dirpath)) == NULL)
	{
		printf("open directory failed");
		close(socket);
	}

	while ((dp = readdir(directory)) != NULL)
	{
		sprintf(path, "%s/%s", dirpath, dp->d_name);

		// conditions to only list regular files and ignore subfolders
		if (stat(path, &buffer) == 0 && S_ISREG(buffer.st_mode))
		{
			count++;
			sent = send(socket, dp->d_name, FILE_NAME_MAX + 1, 0);
			if (sent <= 0)
			{
				printf("Sending file name %s failed.\n", dp->d_name);
				closedir(directory);
				close(socket);
			}
			else
				sentcount++;
		}
	}
	if (sentcount == count)
	{
		strcpy(message, "end of file list.");
		sendAll(socket, message, strlen(message) + 1, 0);
		printf("File list sent to client\n");
		closedir(directory);
	}
	else
	{
		printf("Sending file list not completed.\n");
		closedir(directory);
		close(socket);
	}
}

/*
This function is to ensure all the data in the buffer are sent. It tracks the buffer pointer
and repeat send() call untill all bytes in buffer are sent or error occured 
*/
int sendAll(int socket, void *buffer, size_t length, int flags)
{
	ssize_t n;
	char *p = buffer;
	while (length > 0)
	{
		n = send(socket, p, length, flags);
		if (n <= 0)
			return -1;
		p += n;
		length -= n;
	}
	return 0;
}

/* This function receives and prints a list of recieved file names on the client side*/
void listFileNames(int socket)
{
	int i = 0;
	char file[FILE_NAME_MAX + 1];
	while (recv(socket, file, sizeof(file), 0) > 0)
	{
		i++;
		printf("%s \n", file);
		if (strcmp(file, "end of file list.") == 0)
			break;
	}
}

/* This functions sends the selected file name to server for download*/
void selectFile(int socket, char *filename)
{
	size_t bytessent;
	int message = 0;
	printf("Please enter the file name to download or press enter to exit. \n");
	if (fgets(filename, FILE_NAME_MAX, stdin) != NULL) //read user input
	{
		if (strcmp(filename, "\n") == 0)
		{
			close(socket);
			exit(0);
		}
		filename[strlen(filename)] = '\0';
		printf("Requested file: %s \n", filename);
		
		while (bytessent = send(socket, filename, strlen(filename), 0) > 0)
		{
			message += bytessent;
			if (message = strlen(filename)) // check null terminate
				break;
		}
		if (bytessent <= 0)
		{
			printf("Send file name failed \n");
			close(socket);
			exit(0);
		}

		printf("filename is sent to the server. \n", filename);
	}
	else
	{
		printf("Read file name failed. \n");
		close(socket);
	}
}

/* This functions reads the selected file name and display it on server, and then send file to client*/
void sendFileToClient(int socket)
{
	size_t received;
	int namesize = 0;
	char filename[FILE_NAME_MAX + 1];
	int message;

	while ((received = recv(socket, filename + message, sizeof(filename) - message - 1, 0)) > 0)
	{
		message += received;
		if (message > strlen(filename) || filename[message - 1] == '\n')
			break;
	}
	if (received <= 0)
	{
		printf("Receive filename failed \n");
		close(socket);
	}

	filename[message - 1] = '\0';
	printf("Requested file: %s \n", filename);

	readFileOnServer(filename, socket);
}

/* This functions opens and reads the selected file name on server and send it to client*/
void readFileOnServer(char *filename, int socket)
{
	char filepath[PATH_MAX + 1];
	int filesize;
	int infp;
	int charcount, writecount;
	char buffer[1024], error_line[80];
	DIR *directory;
	char path[PATH_MAX];

	if (realpath(filename, filepath) == NULL)
	{
		printf("%s file path not found. \n", filename);
		close(socket);
	}


	filesize = checkFileSize(filepath);
	printf("%s is %d bytes long. \n", filename, filesize);
	infp = open(filepath, O_RDONLY);
	if (infp == -1)
	{
		sprintf(error_line, "Error in opening file %s for reading ", filename);
		perror(error_line);
		exit(1);
	}
	do
	{
		charcount = read(infp, buffer, sizeof(buffer));
		printf("%zu bytes sending\n", charcount);
		writecount = send(socket, buffer, charcount, 0);

	} while (charcount == sizeof(buffer));
	printf("Finish sending. \n");
	close(infp);
	close(socket);
}

/* This function returns the size of a file */
int checkFileSize(char *filepath)
{
	int size;
	struct stat statbuff;
	stat(filepath, &statbuff);
	size = statbuff.st_size;
	return size;
}

/* This function receives file from server and saves file to local directory*/
void receiveFile(int socketc, char *clientdir, char *filename)
{
	int outfp;
	char buffer[1024], error_line[80];
	int charcount, writecount;
	char path[PATH_MAX + 1];

	sprintf(path, "%s/%s", clientdir, filename);

	outfp = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (outfp == -1)
	{
		sprintf(error_line, "Error in opening file %s for writing \n", filename);
		perror(error_line);
		close(outfp);
		exit(1);
	}
	do
	{
		charcount = recv(socketc, buffer, sizeof(buffer), 0);
		printf("receiving %zu bytes \n", charcount);
		writecount = write(outfp, buffer, charcount);
	} while (charcount == sizeof(buffer));
	
	printf("Finish downloading. \n");
	printf("Disconnecting...\n");
	sleep(3);
	close(outfp);
	close(socketc);
}