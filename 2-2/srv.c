#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define BUF_SIZE 256

void sh_chld(int sig);
void sh_alrm(int sig);

int		client_info(struct sockaddr_in *cliaddr);

int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd;
    int len;
    int port;

    signal(SIGCHLD, sh_chld);
    signal(SIGALRM, sh_alrm);

    server_fd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(atoi(argv[1]));

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(server_fd, 5);

    while (1)
    {
        pid_t pid;
        len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);

        ////// print client information(client IPv4 address, port number) ///////
        if (client_info(&client_addr) < 0)
                write(2, "client_info() err!!\n", strlen("client_info() err!!\n"));

        if ((pid = fork()) < 0)
        {
            write(2, "fork() err!!\n", strlen("fork() err!!\n"));
            exit(1);
        }
        if (pid == 0)
        {
            sprintf(buff, "Child Process ID : %d\n\n", getpid());
            write(1, buff, strlen(buff));
            while (1)
            {
                memset(buff, 0, BUF_SIZE);
                if ((n = read(client_fd, buff, BUF_SIZE) > 0))
                {
                    ////// if received buff was QUIT, close user connection //////
                    if (!strncmp(buff, "QUIT\n", 5))
						break;
                        
                    ///// write received message to client //////
                    write(client_fd, buff, BUF_SIZE);
                }
            }
			alarm(1);
			close(client_fd);
        }
    }
    return 0;
}

void sh_chld(int sig)
{
    printf("Status of Child process was changed.\n");
    wait(NULL);
}

void sh_alrm(int sig)
{
    printf("Child Process(PID : %d) will be terminated.\n", getpid());
	printf("no\n");
    exit(1);
}

////////////////////////////////////////////////////////////////////////
// clinet_info                                                        //
// ================================================================== //
// Input: struct sockaddr_in * -> client address information          //
//                                                                    //
//                                                                    //
//                                                                    //
// Output: int -> 0:success to write                                  //
//               -1:failed to write                                   //
//                                                                    //
// Purpose: print client IP address & port number                     //
////////////////////////////////////////////////////////////////////////

int	client_info(struct sockaddr_in *cliaddr)
{
	char print_buf[BUF_SIZE];
	char buf[BUF_SIZE];

	/////// make client information format ////////
	strcpy(print_buf, "==========Client info==========\n");
	sprintf(buf, "client IP: %s\n\nclient port: %d\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port));
	strcat(print_buf, buf);
	strcat(print_buf, "===============================\n");

	/////// print client information ////////
	if (write(1, print_buf, strlen(print_buf)) < 0)
		return -1;
	return 0;
}