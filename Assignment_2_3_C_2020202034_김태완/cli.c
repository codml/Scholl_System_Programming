////////////////////////////////////////////////////////////////////////
// File Name    :cli.c                                                //
// Date         :2024/05/16                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #2-3: ftp & socket -cli//
// Description  :convert usr command to FTP command, send it          //
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

#define BUF_SIZE 4096

void sh_int(int sig);

int sockfd;

int main(int argc, char **argv)
{
    char buff[BUF_SIZE], write_buff[BUF_SIZE], rcv_buff[BUF_SIZE];
	char *split[256];
	int n;
	int len;
	struct sockaddr_in serv_addr;

	// instruction convert table, except rename //
	char *table[][2] = { {"ls", "NLST"},
				{"dir", "LIST"},
				{"pwd", "PWD"},
				{"cd", "CWD"},
				{"mkdir", "MKD"},
				{"delete", "DELE"},
				{"rmdir", "RMD"},
				{"quit", "QUIT"} };

	///// check the number of arguments /////
	if (argc != 3)
    {
        write(2, "Two arguments are needed: IP, port\n", strlen("Two arguments are needed: IP, port\n"));
        exit(1);
    }

	///// register handling function for signal interrupt /////
	signal(SIGINT, sh_int);

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
		///// reset buffers to avoid overwriting problem /////
		memset(buff, 0, BUF_SIZE);
		memset(write_buff, 0, BUF_SIZE);
		memset(rcv_buff, 0, BUF_SIZE);

		//// reset the number of split ////
		len = 0;

		///// write string which client will send to server /////
		write(STDOUT_FILENO, "> ", 2);
		if ((n = read(STDIN_FILENO, buff, BUF_SIZE)) < 0)
		{
			perror(strerror(errno));
			exit(1);
		}

		//////// split user command by white spaces ///////////////
		for (char *ptr = strtok(buff, " \b\v\f\r\t\n"); ptr; ptr = strtok(NULL, " \b\v\f\r\t\n"))
			split[len++] = ptr;
		split[len] = NULL;

		///////// change usr cmd to ftp cmd(except rename) //////
		for (int i = 0; i < 8; i++)
		{
			if (len != 0 && !strcmp(split[0], table[i][0]))
			{
				strcat(write_buff, table[i][1]);
				break;
			}
		}

		/////// for rename ///////
		if (len != 0 && !strcmp(split[0], "rename"))
		{
			strcat(write_buff, "RNFR");
			/////// give all option && argument to RNFR except last argument ///////
			for (int i = 1; i < len - 1; i++)
			{
				strcat(write_buff, " ");
				strcat(write_buff, split[i]);
			}

			strcat(write_buff, " RNTO ");
			/////// give last argument to RNTO ///////
			strcat(write_buff, split[len - 1]);
		}
		else
		{
			///////Unknown Instruction///////
			if (!write_buff[0]) 
				strcat(write_buff, "UNKNOWN");
			///////option && argument///////
			for (int i = 1; i < len; i++)
			{
				////// cd .. -> not CWD, CDUP //////
				if (!strncmp(write_buff, "CWD", 3) && !strcmp(split[i], ".."))
				{
					memmove(write_buff + 1, write_buff, strlen(write_buff));
					strncpy(write_buff, "CDUP", strlen("CDUP"));
					continue;
				}
				strcat(write_buff, " ");
				strcat(write_buff, split[i]);
			}
		}
		
		////// write string to server //////
		if (write(sockfd, write_buff, strlen(write_buff)) > 0)
		{
			///// write QUIT -> exit /////
			if (!strcmp(write_buff, "QUIT"))
				exit(0);
			
			//// if success to write, read from server /////
			if (read(sockfd, rcv_buff, BUF_SIZE) > 0)
				write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
			else //// if disconnect from server process, read will return 
				break;
		}
		else
			break;
	}

	///// close socket connected with server /////
	close(sockfd);
	return 0;
}

void sh_int(int sig)
{
	///// if receive ctrl+C, send "QUIT" to server and quit /////
	write(sockfd, "QUIT", 4);
	close(sockfd);
	exit(0);
}