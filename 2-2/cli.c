////////////////////////////////////////////////////////////////////////
// File Name    :cli.c                                                //
// Date         :2024/05/04                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #2-2: fork()           //
// Description  :client with fork()                                   //
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

int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
	int n;
	int sockfd;
	struct sockaddr_in serv_addr;

	if (argc != 3)
    {
        write(2, "Two arguments are needed: IP, port\n", strlen("Two arguments are needed: IP, port\n"));
        exit(1);
    }

	////// make socket: domain: IPv4, type: stream(TCP) //////
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	////// set server address structure(including information) //////
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	///// connect to server using serv_addr. if failed, write error and exit /////
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		write(2, "connect error\n", strlen("connect error\n"));
		exit(1);
	}
		
	while (1)
	{
		///// reset buffer to avoid overwriting problem /////
		memset(buff, 0, BUF_SIZE);

		///// write string which client will send to server /////
		write(STDOUT_FILENO, "> ", 2);
		read(STDIN_FILENO, buff, BUF_SIZE);

		////// write string to server //////
		if (write(sockfd, buff, BUF_SIZE) > 0)
		{
			//// if success to write, read from server /////
			if (read(sockfd, buff, BUF_SIZE) > 0)
				printf("from server:%s", buff);
			//// if disconnect from server process, read will return 
			else
				break;
		}
		else
			break;
	}

	///// close socket connected with server /////
	close(sockfd);
	return 0;
}
