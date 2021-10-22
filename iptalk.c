/*iptalk.c
An IP-based talk program using a telephone call paradigm
This version uses two processes: main process accepts connections and 
receives traffic from client. Child process sends traffic to client. 
A better implementation would fork both listener and talker from the 
connected forked process, allowing multiple simultaneousd connections
each with two processes + the original parent process accepting more connections. 

Author:  James Hamlyn-Harris
Last updated: 21-05-2019*/

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

/*macros*/
#define SIZE sizeof(struct sockaddr_in)
#define HOST "localhost"
#define PORT 7001

/*function prototypes*/
void client();
void server();
void catcher(int sig);

int newsockfd, sockfd, i;
char c, rc;
int pid;


static struct sigaction act;
struct sockaddr_in serverc = {AF_INET, PORT};
struct sockaddr_in servers = {AF_INET, PORT, INADDR_ANY};
struct hostent* hp;



/*main function for managing command-line options (client or server)
@param argc integer representing number of command line arguments including program name
@param argv Char array containing command line arguments*/
int main(int argc,char *argv[])
{
    char address[20];
    char *iaddress = &address[0];
    int newport;

    /*find the ip address of host just in case it changes)*/
    hp=gethostbyname(HOST);
    bcopy(hp->h_addr, &serverc.sin_addr, hp->h_length);
    strcpy(iaddress, (char*)inet_ntoa(serverc.sin_addr));
  

    if(argc==4)
    {
        if(sscanf(argv[3], "%d", &newport)==1) /*This option allows pairs of users to set up a private*/
        {                                      /* conversation by specifying a different port number. */
            serverc.sin_port = newport;
            servers.sin_port = newport;
            printf("Using port number %d\n", servers.sin_port);
        }
        else
        {
            printf("Using default port number %d\n", servers.sin_port);
        }
    }

    if(argc == 3 || argc==4) /*This option allows the program to be used on a server other than default.*/
    {                        /*The gethostname() is not used as this is unreliable in linux and tends to*/
                             /*return a truncated version of the host address. */
        strcpy(iaddress, argv[2]);
	serverc.sin_addr.s_addr=inet_addr(iaddress);
        printf("Using server at %s\n", iaddress);
    }
    else
        printf("Using server at %s\n", iaddress);

    if (argc>=2 && argc <= 4)
    {
        if(strcmp(argv[1], "server")==0 )
            server();
        else
	{
            	client();
	}
        exit(0);
    }


    /*else invalid input*/
    
    printf("usage: %s server|client [ip_address [port]]\n", argv[0]);
    printf("where port is a port number (default port: %d).\n", PORT );
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

	if ((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1)
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

	while(1)/*accept connections*/
	{
		//NULL for sockaddr means that client IP is discarded. 
		if ((newsockfd = accept(sockfd, NULL, NULL)) == -1) 
		{
			perror("Accept error: ");
			continue;
		}
	
		pid=fork();
		if (pid > 0)/*listener*/
		while(1)
		{
			if (recv(newsockfd, &rc, 1, 0) > 0)
				if (rc=='\n')
                    printf("\t(from client)\n");
                else
                    printf("%c", toupper(rc));
			else
			{
				printf("Caller has hung up\n");
				close(newsockfd);
				kill(pid, SIGKILL);/*kill the talker*/
				/*break; use instead of exit(0) to stay listening for futher calls*/
                		exit(0);
			}
		}
		else /*talker*/
		{
			c='\n';
			while(1)
			{
				c=getchar();
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


	if ((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1)
	{
		perror("Socket failed: ");
		exit(0);
	}

	if (connect(sockfd, (struct sockaddr *)&serverc, SIZE) == -1)
	{
		perror("Connect failed: ");
		exit(0);
	}

	pid=fork();
	if (pid > 0)/*listener*/
	while(1)
	{
		if (recv(sockfd, &rc, 1, 0) > 0)
		    if (rc=='\n')
                printf("\t(from server)\n");
            else
                printf("%c", toupper(rc));
		else
		{
			printf("Server has disconnected\n");
			close(sockfd);
			kill(pid, SIGKILL);/*kill the talker*/
			exit(0);
		}
	}
	else /*talker*/
	{
		c='\n';
		while(1)
		{
			c=getchar();
			send(sockfd, &c, 1, 0);	
		}
	}
}

