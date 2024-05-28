////////////////////////////////////////////////////////////////////////
// File Name    :cli.c                                                //
// Date         :2024/05/29                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #3-2: data connection  //
// Description  :make control & data connection                       //
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
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#define BUF_SIZE 4096

void convert_addr_to_str(char *buf, struct sockaddr_in *tmp);

void main(int argc, char **argv)
{
    char				*hostport;
	char 				buff[BUF_SIZE], portcmd[BUF_SIZE], cmd[BUF_SIZE];
    int 				ctrlfd, datafd, dataconfd;
	int					n, port, bytes;
    struct sockaddr_in	temp;
	int					len;
	char				*split[256];

    ///// check the number of arguments /////
	if (argc != 3)
    {
        write(2, "Two arguments are needed: IP, port\n", strlen("Two arguments are needed: IP, port\n"));
        exit(1);
    }

	//// control connection socket ////
    ctrlfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&temp, 0, sizeof(temp));
	temp.sin_family=AF_INET;
	temp.sin_addr.s_addr=inet_addr(argv[1]);
	temp.sin_port=htons(atoi(argv[2]));

    ///// connect to server using serv_addr. if failed, write error and exit /////
	if (connect(ctrlfd,(struct sockaddr *)&temp,sizeof(temp)) < 0)
	{
		perror("connect error");
		exit(1);
	}
    while (1)
    {
		memset(buff, 0, BUF_SIZE);

        write(STDOUT_FILENO, "> ", 2);
		///// read user command from stdin /////
        if ((n = read(STDIN_FILENO, buff, BUF_SIZE)) < 0)
        {
            perror("Read error");
            exit(1);
        }
		buff[n - 1] = '\0'; // erase '\n'

		len = 0;
		//////// split user command by white spaces ///////////////
		for (char *ptr = strtok(buff, " \b\v\f\r\t\n"); ptr; ptr = strtok(NULL, " \b\v\f\r\t\n"))
			split[len++] = ptr;
		split[len] = NULL;

		/// if read invalid command, repeat read from stdin /// 
		if (len == 0 || (strcmp(split[0], "ls") && strcmp(split[0], "quit")))
			continue;
		/////// ls -> NLST ///////
		if (!strcmp(split[0], "ls"))
			strcpy(cmd, "NLST");
		/////// quit -> QUIT ////////
		else if (!strcmp(split[0], "quit"))
			strcpy(cmd, "QUIT");
		////// send options & arguments of commands too //////
		for (int i = 1; i < len; i++)
		{
			strcat(cmd, " ");
			strcat(cmd, split[i]);
		}

		//// if received quit, don't make data connection and just send FTP command ////
		if (!strcmp(cmd, "QUIT"))
		{
			if (write(ctrlfd, cmd, strlen(cmd)) < 0)
			{
				perror("write error");
				exit(1);
			}

			//// read "221 ..." from server ////
			if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
			{
				perror("read error");
				exit(1);
			}
			buff[n] = 0;

			////// print received string //////
			printf("%s\n", buff);

			//// close control connection ////
			close(ctrlfd);
			return ;
		}

		///// make random port num(10001 ~ 30000) /////
		srand(time(NULL));
		port = 10001 + rand() % 20000;

		memset(&temp, 0, sizeof(temp));
		temp.sin_family=AF_INET;
		temp.sin_addr.s_addr=htonl(INADDR_LOOPBACK); //// loopback == 127.0.0.1 ////
		temp.sin_port=htons(port);

		/////// make socket for data connection ///////
		datafd = socket(AF_INET, SOCK_STREAM, 0);
		bind(datafd, (struct sockaddr *)&temp, sizeof(temp));
		listen(datafd, 5);

		/////// make PORT instruction and write to server //////
        convert_addr_to_str(portcmd, &temp);
		if (write(ctrlfd, portcmd, strlen(portcmd)) < 0)
		{
			perror("write error");
			exit(1);
		}

		n = sizeof(temp);
		if ((dataconfd = accept(datafd, (struct sockaddr *)&temp, &n)) < 0)
		{
			perror("data connection error");
			exit(1);
		}
		close(datafd);

		if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
		{
			perror("read error");
			exit(1);
		}
		buff[n] = '\0';
		printf("%s\n", buff);

		if (write(ctrlfd, cmd, strlen(cmd)) < 0)
		{
			perror("write error");
			exit(1);
		}

		if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
		{
			perror("read error");
			exit(1);
		}
		buff[n] = '\0';
		printf("%s\n", buff);

		if ((n = read(dataconfd, buff, BUF_SIZE)) < 0)
		{
			perror("read error");
			exit(1);
		}
		buff[n] = '\0';
		write(STDOUT_FILENO, buff, strlen(buff));
		bytes = n;

		if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
		{
			perror("read error");
			exit(1);
		}
		buff[n] = '\0';
		printf("%s\n", buff);
		
		if (!strncmp(buff, "226", 3))
			printf("OK. %d bytes is received.\n", bytes);

		close(dataconfd);
    }
}

////////////////////////////////////////////////////////////////////////
// convert_addr_to_str                                                //
// ================================================================== //
// Input: char * -> buffer to store result message(PORT ip,port)      //
//        struct sockaddr_in * -> storing ip, port num                //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: make PORT instruction: separate ip & port in bytes        //
////////////////////////////////////////////////////////////////////////

void convert_addr_to_str(char *buf, struct sockaddr_in *tmp)
{
	char *bu, *ptr;

	strcpy(buf, "PORT ");
	strcat(buf, inet_ntoa(tmp->sin_addr));
	bu = buf;
	while (ptr = strchr(buf, '.'))
		*ptr = ',';
	sprintf(buf + strlen(buf), ",%d,%d", ntohs(tmp->sin_port) >> 8, ntohs(tmp->sin_port) & 0xFF);
}