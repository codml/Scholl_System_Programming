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
#include <sys/stat.h>

#define BUF_SIZE 4096

void convert_addr_to_str(char *buf, struct sockaddr_in *addr);
void log_in(int sockfd);

void main(int argc, char **argv)
{
    char				*hostport;
	char 				buff[BUF_SIZE], portcmd[BUF_SIZE], cmd[BUF_SIZE];
    int 				ctrlfd, datafd, dataconfd;
	int					n, port, bytes;
    struct sockaddr_in	temp;
	int					len;
	char				*split[256];

	// instruction convert table, except rename //
	char *table[][2] = { {"ls", "NLST"},
				{"dir", "LIST"},
				{"pwd", "PWD"},
				{"cd", "CWD"},
				{"mkdir", "MKD"},
				{"delete", "DELE"},
				{"rmdir", "RMD"},
				{"quit", "QUIT"},
				{"get", "RETR"},
				{"put", "STOR"},
				{"bin", "TYPE I"},
				{"ascii", "TYPE A"},
				{NULL, NULL}};

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

	log_in(ctrlfd);
    while (1)
    {
		memset(buff, 0, BUF_SIZE);

        write(STDOUT_FILENO, "ftp> ", 5);
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
		if (len == 0)
			continue;
		///////// buf initialization //////////
		memset(cmd, 0, sizeof(cmd));

		///////// change usr cmd to ftp cmd(except rename) //////
		for (int i = 0; table[i][0]; i++)
		{
			if (!strcmp(split[0], table[i][0]))
			{
				strcat(cmd, table[i][1]);
				break;
			}
		}

		/////// for rename ///////
		if (!strcmp(split[0], "rename") && len == 3)
		{
			strcat(cmd, "RNFR ");
			/////// give old name to RNFR ///////
			strcat(cmd, split[1]);

			write(ctrlfd, cmd, strlen(cmd));
			if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
			{
				perror("read error");
				exit(1);
			}
			buff[n] = '\0';
			write(STDOUT_FILENO, buff, strlen(buff));

			if (!strncmp("550", buff, 3))
				continue;

			strcpy(cmd, "RNTO ");
			/////// give new name to RNTO ///////
			strcat(cmd, split[2]);
			write(ctrlfd, cmd, strlen(cmd));
			if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
			{
				perror("read error");
				exit(1);
			}
			buff[n] = '\0';
			write(STDOUT_FILENO, buff, strlen(buff));
			continue;
		}
		else if (!strcmp(split[0], "type") && len == 2)
		{
			if (!strcmp(split[1], "ascii"))
				strcat(cmd, "TYPE A");
			else if (!strcmp(split[1], "binary"))
				strcat(cmd, "TYPE I");
			else
				continue;
		}
		else
		{
			if (!cmd[0])
				continue;
			///////option && argument///////
			for (int i = 1; i < len; i++)
			{
				////// cd .. -> not CWD, CDUP //////
				if (!strcmp(split[0], "cd") && !strcmp(split[i], ".."))
				{
					memmove(cmd + 1, cmd, strlen(cmd));
					strncpy(cmd, "CDUP", strlen("CDUP"));
					continue;
				}
				strcat(cmd, " ");
				strcat(cmd, split[i]);
			}
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
			write(STDOUT_FILENO, buff, strlen(buff));

			//// close control connection ////
			close(ctrlfd);
			return ;
		}
		if (!strncmp(cmd, "NLST", 4) || !strncmp(cmd, "LIST", 4) || !strncmp(cmd, "RETR", 4) || !strncmp(cmd, "STOR", 4))
		{
			///// make random port num(10001 ~ 30000) /////
			srand(time(NULL));
			port = 10001 + rand() % 50000;

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

			/////// wait server's data connection requirement /////
			n = sizeof(temp);
			if ((dataconfd = accept(datafd, (struct sockaddr *)&temp, &n)) < 0)
			{
				perror("data connection error");
				exit(1);
			}
			close(datafd); // data connection -> one send, and close

			//// read 200 or 550 response and print ////
			if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
			{
				perror("read error");
				exit(1);
			}
			buff[n] = '\0';
			write(STDOUT_FILENO, buff, strlen(buff));

			/// if connection failed, exit program ///
			if (!strncmp(buff, "550", 3))
			{
				close(dataconfd);
				close(ctrlfd);
				exit(0);
			}

			//// send FTP command (NLST or RETR or STOR) ////
			if (write(ctrlfd, cmd, strlen(cmd)) < 0)
			{
				perror("write error");
				exit(1);
			}

			///// read '150 opening ....' from server /////
			if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
			{
				perror("read error");
				exit(1);
			}
			buff[n] = '\0';
			write(STDOUT_FILENO, buff, strlen(buff));

			if (!strncmp(cmd, "RETR", 4) || !strncmp(cmd, "STOR", 4))
			{
				if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
				{
					perror("read error");
					exit(1);
				}
				buff[n] = '\0';

				if (!strncmp(buff, "550", 3))
				{
					write(STDOUT_FILENO, buff, strlen(buff));
					close(dataconfd);
					continue;
				}

				struct stat infor;
				char file_name[BUF_SIZE];
				if ((stat(buff, &infor) == -1 && !strncmp(cmd, "RETR", 4))
						|| (stat(buff, &infor) == 0 && !strncmp(cmd, "STOR", 4)))
					write(ctrlfd, "OK", 2);
				else
					write(ctrlfd, "NO", 2);

				strcpy(file_name, buff);

				if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
				{
					perror("read error");
					exit(1);
				}
				buff[n] = '\0';
				
				if (!strncmp(buff, "550", 3))
				{
					write(STDOUT_FILENO, buff, strlen(buff));
					close(dataconfd);
					continue;
				}

				if (!strncmp(cmd, "RETR", 4))
				{
					if ((n = read(dataconfd, buff, BUF_SIZE)) < 0)
					{
						perror("read error");
						exit(1);
					}
					int file_fd = open(file_name, O_WRONLY | O_CREAT);
					write(file_fd, buff, strlen(buff));
					close(file_fd);
				}
				else
				{
					int file_fd = open(file_name, O_RDONLY);
					n = read(file_fd, buff, BUF_SIZE);
					buff[n] = '\0';
					close(file_fd);

					if ((n = write(dataconfd, buff, strlen(buff))) < 0)
					{
						perror("write error");
						exit(1);
					}
				}
				bytes = n;
				if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
				{
					perror("read error");
					exit(1);
				}
				buff[n] = '\0';

				write(STDOUT_FILENO, buff, strlen(buff));
				write(STDOUT_FILENO, "\n", 1);
				if (!strncmp(cmd, "RETR", 4))
					sprintf(buff, "OK. %d bytes is received.\n", bytes);
				else
					sprintf(buff, "OK. %d bytes is sent.\n", bytes);
				write(STDOUT_FILENO, buff, strlen(buff));
				close(dataconfd);
			}
			else
			{
				/////// read command result from server via data connection ///////
				if ((n = read(dataconfd, buff, BUF_SIZE)) < 0)
				{
					perror("read error");
					exit(1);
				}
				buff[n] = '\0';

				/////// print the result and store bytes //////
				write(STDOUT_FILENO, buff, strlen(buff));
				bytes = n;

				/////// read '226....' from server //////
				if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
				{
					perror("read error");
					exit(1);
				}
				buff[n] = '\0';

				write(STDOUT_FILENO, buff, strlen(buff));
				
				//////////// if succeed to receive in server, print OK & #bytes ////////////
				if (!strncmp(buff, "226", 3))
				{
					write(STDOUT_FILENO, "\n", 1);
					sprintf(buff, "OK. %d bytes is received.\n", bytes);
					write(STDOUT_FILENO, buff, strlen(buff));
				}

				////// close data connection //////
				close(dataconfd);
			}
		}
		else
		{
			//// send FTP command ////
			if (write(ctrlfd, cmd, strlen(cmd)) < 0)
			{
				perror("write error");
				exit(1);
			}

			/////// read Success or fail message from server //////
			if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
			{
				perror("read error");
				exit(1);
			}
			buff[n] = '\0';
			write(STDOUT_FILENO, buff, strlen(buff));
		}
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

void convert_addr_to_str(char *buf, struct sockaddr_in *addr)
{
	char *ptr, *tmp;

	strcpy(buf, "PORT "); // add PORT to command
	strcat(buf, inet_ntoa(addr->sin_addr)); // add ip address string to command
	tmp = buf;
	for (tmp = buf; ptr = strchr(tmp, '.'); tmp = ptr) // convert '.' to ','
		*ptr = ',';

	////// split port into bytes(2568 -> 10*2^8 + 8 -> 10,8)
	sprintf(buf + strlen(buf), ",%d,%d", ntohs(addr->sin_port) >> 8, ntohs(addr->sin_port) & 0xFF);
}

////////////////////////////////////////////////////////////////////////
// log_in                                                             //
// ================================================================== //
// Input: int -> socket descriptor that is used for server connection //
//                                                                    //
// Output: none                                                       //
//                                                                    //
// Purpose: check connection to server, send ID & passwd and receive  //
//				answer from server                                    //
////////////////////////////////////////////////////////////////////////

void log_in(int sockfd)
{
    int n;
    char tmp_buff[1024], *passwd, buf[BUF_SIZE];

	// read REJECTION or ACCEPTED -> ip access //
    if ((n = read(sockfd, buf, BUF_SIZE)) <= 0)
	{
		perror("Read error");
		exit(1);
	}
    buf[n] = '\0';
    if (!strncmp(buf, "431", 3)) // rejected from server
    {
		write(STDOUT_FILENO, buf, strlen(buf));
		close(sockfd);
		exit(1);
	}
	else // receive ACCEPTED
		write(STDOUT_FILENO, buf, strlen(buf));

    while (1) // read ID & passwd from STDIN_FILENO and send to server, then receive OK, FAIL, or DISCONNECTION //
    {
		write(STDOUT_FILENO, "Name : ", strlen("Name : "));
		if ((n = read(STDIN_FILENO, tmp_buff, 1024)) <= 0) // if read failed, exit
			exit(1);
		tmp_buff[n - 1] = '\0'; // store input except line feed('\n')
		sprintf(buf, "USER %s", tmp_buff);
		if (write(sockfd, buf, strlen(buf)) <= 0) // write ID to server
			exit(1);

		if ((n = read(sockfd, buf, BUF_SIZE)) <= 0) // receive result from server
			exit(1);
		buf[n] = '\0';
		write(STDOUT_FILENO, buf, strlen(buf));
		if (!strncmp(buf, "430", 3))
			continue;
		else if (!strncmp(buf, "530", 3))
		{
			close(sockfd);
			exit(0);
		}

		passwd = getpass("Passwd : "); // receive passwd encrypted
		sprintf(buf, "PASS %s", passwd);
		if (write(sockfd, buf, strlen(buf)) <= 0) // write to server
			exit(1);

		if ((n = read(sockfd, buf, BUF_SIZE)) <= 0) // receive result from server
			exit(1);
		buf[n] = '\0';
        write(STDOUT_FILENO, buf, strlen(buf));
		if (!strncmp(buf, "430", 3))
			continue;
		else if (!strncmp(buf, "530", 3))
		{
			close(sockfd);
			exit(0);
		}
		else
			return ;
    }
}