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

/*other macros*/
#define SIZE sizeof(struct sockaddr_in)
#define HOST "localhost"
#define PORT 7001
#define PATH_MAX 4096
#define FILE_NAME_MAX 255

/*function prototypes*/
void client();
void server();
void catcher(int sig);

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

	/*find the ip address of host just in case it changes)*/
	hp = gethostbyname(HOST);
	bcopy(hp->h_addr_list[0], &serverc.sin_addr, hp->h_length); // copy host address to IP socket address
	strcpy(iaddress, (char *)inet_ntoa(serverc.sin_addr));		// convert from binary to string form and save to the memory

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
			server();
		else
		{
			client();
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
void server()
{
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

		pid = fork();
		if (pid > 0) /*listener*/

			//Task after establishing connection
			while (1)
			{
				if (recv(newsockfd, &rc, 1, 0) > 0)
					if (rc == '\n')
						printf("\t(from client)\n");
					else
						printf("%c", toupper(rc));
				else
				{
					printf("Caller has hung up\n");
					close(newsockfd);
					kill(pid, SIGKILL); /*kill the talker*/
										/*break; use instead of exit(0) to stay listening for futher calls*/
					exit(0);
				}
			}
		else /*talker*/
		{
			c = '\n';
			while (1)
			{
				c = getchar();
				send(newsockfd, &c, 1, 0);
			}
		}
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
void client()
{
	printf("Running as client. \t ^C to hangup. \n");

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

	pid = fork();
	if (pid > 0) /*listener*/
		while (1)
		{
			if (recv(sockfd, &rc, 1, 0) > 0)
				if (rc == '\n')
					printf("\t(from server)\n");
				else
					printf("%c", toupper(rc));
			else
			{
				printf("Server has disconnected\n");
				close(sockfd);
				kill(pid, SIGKILL); /*kill the talker*/
				exit(0);
			}
		}
	else /*talker*/
	{
		c = '\n';
		while (1)
		{
			c = getchar();
			send(sockfd, &c, 1, 0);
		}
	}
}

/* This function sends a list of regular files in the specified directory, ignoring subfolders*/
void sendDirList(char *dirpath, int socket)
{
	DIR *directory;
	struct dirent *dp;
	size_t namelen;
	char path[PATH_MAX];
	struct stat buffer;
	int count = 0;
	int sentcount = 0;
	char message[30];

	if (directory = opendir(dirpath) == NULL)
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
			//namelen = namelen + strlen(dp->d_name);
			if (sendAll(socket, dp->d_name, strlen(dp->d_name) + 1, 0) != 0)
			{
				printf("Sending file name %s failed.\n", dp->d_name);
				close(socket);
			}
			else
				sentcount++;
		}
	}
	if (sentcount == count)
	{
		message[30] = "end of file list";
		sendAll(socket, message, strlen(message) + 1, 0);
	}
}

/*
This function is to ensure all the data in the buffer are sent. It tracks the buffer pointer
and repeat send() call untill all bytes in buffer are sent or error occured 
*/
int sendAll(int socket, const void *buffer, size_t length, int flags)
{
	ssize_t n;
	const char *p = buffer;
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

/*
This function is to ensure all the data in the buffer are received. It tracks the buffer pointer
and repeat send() call untill all bytes in buffer are sent or error occured 
*/
int receiveAll(int socket, const void *buffer, size_t length, int flags)
{
	ssize_t n;
	const char *p = buffer;
	while (length > 0)
	{
		n = recv(socket, p, length, flags);
		if (n <= 0)
			return -1;
		p += n;
		length -= n;
	}
	return 0;
}

/* This function prints a list of recieved file names on the screen*/
void listFileNames(int socket)
{
	int i = 0;
	char filename[FILE_NAME_MAX + 1];
	while (recv(socket, filename, sizeof(filename), 0) > 0)
	{
		i++;
		printf("%d. %s", i, filename);
		if (strcomp(filename, "end of file list") == 0)
			break;
	}
}

/* This functions sends the selected file name to download*/
void selectFile(int socketc)
{
	char filename[FILE_NAME_MAX+1];
	if (fgets(filename, sizeof(filename), stdin))
	{
		if (sendAll(socketc, filename, strlen(filename), 0) < 0)
		{
			puts("Send file name failed");
			close(socket);
		}
	}
	else
	{
		printf("read file name failed.");
		close(socket);
	}
}

void readFileOnServer()
{
	char filepath[PATH_MAX + 1];
	filepath[PATH_MAX + 1] = "";
}

void copyFileToClient()
{
}