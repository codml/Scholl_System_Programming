////////////////////////////////////////////////////////////////////////
// File Name    :cli.c                                                //
// Date         :2024/05/01                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #2-1: socket           //
// Description  :socket programming - client: send FTP commans        //
//                                            and receive result      //
////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#define MAX_BUFF 4096
#define RCV_BUFF 16384

void	conv_cmd(char *buff, char *cmd_buff);
void    process_result(char *buf);

void main(int argc, char **argv)
{
    int     sockfd, len;
    struct	sockaddr_in server_addr;
	char	*str;

    char    buff[MAX_BUFF], cmd_buff[MAX_BUFF], rcv_buff[RCV_BUFF];
    int     n;

	//////// check # of arguments ///////// 
	if (argc != 3)
	{
		write(2, "Format: ./cli [server addr] [port num]\n", strlen("Format: ./cli [server addr] [port num]\n"));
		exit(1);
	}

	///////// make socket for client //////////
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		write(2, "can't create socket\n", strlen("can't create socket\n"));
		exit(1);
	}

	///////// set server address(address family, IPv4 address, port number) //////////
	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));

	///////// connect to server process //////////
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		write(2, "can't connect\n", strlen("can't connect\n"));
		exit(1);
	}

	///////// reset buffers for read from user, write to server(FTP command), read from server(command result) ////////
	memset(buff, 0, sizeof(buff));
	memset(cmd_buff, 0, sizeof(cmd_buff));
	memset(rcv_buff, 0, sizeof(rcv_buff));
	write(1, "> ", 2);
	
	//////////// read user command until failed to read or meet EOF(ctrl+D) ///////////
    while (1)
    {
		if ((n = read(0, buff, sizeof(buff))) < 0)
		{
			write(2, "read() error!!\n", strlen("read() error!!\n"));
			exit(1);
		}
		buff[n] = '\0'; // null-terminated

		//////// convert user command to FTP command ////////
        conv_cmd(buff, cmd_buff);

		//////// write FTP command to server socket(=server process) ///////
		n = strlen(cmd_buff);
        if (write(sockfd, cmd_buff, n) != n)
        {
			write(2, "write() error!!\n", strlen("write() error!!\n"));
			exit(1);
		}

		////// read command result from server ///////
		if ((n = read(sockfd, rcv_buff, RCV_BUFF - 1)) < 0)
		{
			write(2, "read() error\n", strlen("read() error\n"));
			exit(1);
		}
		rcv_buff[n] = '\0'; // null-terminated

		/////// if receive QUIT, quit program //////
		if (!strcmp(rcv_buff, "QUIT"))
		{
			write(1, "Program quit!!\n", strlen("Program quit!!\n"));
			close(sockfd);
			exit(0);
		}

		//////// print the received result ////////
		process_result(rcv_buff);
		//////// reset buffers for next while() ////////
		memset(buff, 0, sizeof(buff));
		memset(cmd_buff, 0, sizeof(cmd_buff));
		memset(rcv_buff, 0, sizeof(rcv_buff));
		write(1, "> ", 2);
    }
	/////// if meet EOF, close socket descriptor ///////
	close(sockfd);
}

////////////////////////////////////////////////////////////////////////
// conv_cmd                                                           //
// ================================================================== //
// Input: char * -> read from user(user command)                      //
//        char * -> memory to store FTP command converted from buff   //
//                                                                    //
//                                                                    //
// Output: void                                                       //
//                                                                    //
// Purpose: convert user command(ls, quit) to FTP command(NLST, QUIT) //
////////////////////////////////////////////////////////////////////////

void	conv_cmd(char *buff, char *cmd_buff)
{
	int		len = 0;
	char	*split[256];

	//////// split user command by white spaces ///////////////
    for (char *ptr = strtok(buff, " \b\v\f\r\t\n"); ptr; ptr = strtok(NULL, " \b\v\f\r\t\n"))
		split[len++] = ptr;
	split[len] = NULL;
	/////// ls -> NLST ///////
	if (!strcmp(split[0], "ls"))
		strcpy(cmd_buff, "NLST");
	/////// quit -> QUIT ////////
	else if (!strcmp(split[0], "quit") && len == 1)
		strcpy(cmd_buff, "QUIT");
	////// unknown command -> same //////
	else
		strcpy(cmd_buff, split[0]);
	////// send options & arguments of commands too //////
	for (int i = 1; i < len; i++)
	{
		strcat(cmd_buff, " ");
		strcat(cmd_buff, split[i]);
	}
}

////////////////////////////////////////////////////////////////////////
// process_result                                                     //
// ================================================================== //
// Input: char * -> command result from server                        //
//                                                                    //
//                                                                    //
//                                                                    //
// Output: none                                                       //
//                                                                    //
//                                                                    //
// Purpose: write command result                                      //
////////////////////////////////////////////////////////////////////////

void process_result(char *buf)
{
	//////// print command result ///////
	if (write(1, buf, strlen(buf)) < 0)
	{
		write(2, "write() error\n", strlen("write() error\n"));
		exit(1);
	}
}