////////////////////////////////////////////////////////////////////////
// File Name    :srv.c                                                //
// Date         :2024/05/04                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #2-2: fork()           //
// Description  :server with fork(): parent -> wait client access     //
//                                   child -> write back client msg   //
////////////////////////////////////////////////////////////////////////

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

void    sh_chld(int sig);
void    sh_alrm(int sig);
int     client_info(struct sockaddr_in *cliaddr);

int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd;
    int len;
    int port;

    //// register functions which is called when signal occurred //////
    signal(SIGCHLD, sh_chld);
    signal(SIGALRM, sh_alrm);

    ////// make socket for server //////
    server_fd = socket(PF_INET, SOCK_STREAM, 0);

    ////// set server address for bind //////
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(atoi(argv[1]));

    ///// bind server address with server socket /////
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    ////// open queue for client connect /////
    listen(server_fd, 5);

    while (1)
    {
        pid_t pid;
        len = sizeof(client_addr);
        /////// server accept client's connect, get client address & port ///////
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);

        ////// print client information(client IPv4 address, port number) ///////
        if (client_info(&client_addr) < 0)
                write(2, "client_info() err!!\n", strlen("client_info() err!!\n"));

        ////// fork: make child process (+if error, exit program) //////
        if ((pid = fork()) < 0)
        {
            write(2, "fork() err!!\n", strlen("fork() err!!\n"));
            exit(1);
        }

        /////// pid == 0 -> child process: read from client and write the same to client //////
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
						kill(getpid(), SIGALRM);
                    /////// when child process ends, call SIGALRM by raise() ///////

                    ///// write received message to client //////
                    write(client_fd, buff, BUF_SIZE);
                }
            }
        }
        ////// close connection with client in parent process //////
        close(client_fd);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////
// sh_chld                                                            //
// ================================================================== //
// Input: int -> signal                                               //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: called when SIGCHLD occured                               //
////////////////////////////////////////////////////////////////////////

void sh_chld(int sig)
{
    ///// when SIGCHLD occurred, this function is called /////
    printf("Status of Child process was changed.\n");

    ///// wait for state changes in a child of calling process /////
    wait(NULL);
}

////////////////////////////////////////////////////////////////////////
// sh_alrm                                                            //
// ================================================================== //
// Input: int -> sig                                                  //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: called when SIGALRM occured                               //
////////////////////////////////////////////////////////////////////////

void sh_alrm(int sig)
{
    ///// when SIGALRM occurred, this function is called /////
    printf("Child Process(PID : %d) will be terminated.\n", getpid());

    ///// terminate child process //////
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