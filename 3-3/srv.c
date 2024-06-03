////////////////////////////////////////////////////////////////////////
// File Name    :srv.c                                                //
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
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <fcntl.h>

#define BUF_SIZE 4096
#define TMP_SIZE 1024
#define MAX_BUF 4096

#define FLAGS (O_RDWR | O_CREAT | O_TRUNC)
#define BIN_MODE (S_IXUSR | S_IXGRP | S_IXOTH)
#define ASCII_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define START 0
#define ILLEGAL 1
#define AUTH 2
#define FTP 3
#define RESULT 4
#define BYTE_RESULT 5
#define DISCONNECT 6
#define TERM 7

char	*g_ip;
int		g_port;
char	g_user[256] = "None";
char	g_mode = 'b';
time_t	g_time;
int		log_fd;

void	sh_int(int sig);
void	convert_str_to_addr(char *str, struct sockaddr_in *addr);
int		log_auth(int connfd);
int		user_match(char *user, char *passwd);
int		NLST(char *buf, char *print_buf);
void	MtoS(struct stat *infor, const char *pathname, char *print_buf);
int		LIST(char *buf, char *print_buf);
int		PWD(char *buf, char *print_buf);
int		CWD(char *buf, char *print_buf);
int		CDUP(char *buf, char *print_buf);
int		MKD(char *buf, char *print_buf);
int		DELE(char *buf, char *print_buf);
int		RMD(char *buf, char *print_buf);
int		RNFR(char *buf, char *name_from);
int		RNTO(char *buf, char *name_from);
void	write_log(int fd, char *command, int bytes, int type);

void main(int argc, char **argv)
{
    char buff[BUF_SIZE], send_buff[BUF_SIZE], tmp_buff[TMP_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd, data_fd;
    int len;
	FILE *fp_checkIP, *fp_motd;

	signal(SIGINT, sh_int);
	log_fd = open("logfile", (O_RDWR | O_CREAT | O_APPEND), ASCII_MODE);
	write_log(log_fd, NULL, 0, START);
    ///// check the number of arguments /////
    if (argc != 2)
    {
        write(2, "One argument is needed: port\n", strlen("one argument is needed: port\n"));
        raise(SIGINT);
    }

    ////// make socket for server //////
    server_fd = socket(PF_INET, SOCK_STREAM, 0);

    ////// set server address for bind //////
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(atoi(argv[1]));

	///// bind server address with server socket /////
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind error");
        raise(SIGINT);
    }

    ////// open queue for client connect /////
    listen(server_fd, 5);

    len = sizeof(client_addr);
	while (1)
	{
		/////// server accept client's connect, get client address & port ///////
		if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len)) < 0)
		{
			perror("control accept error");
			raise(SIGINT);
		}
		
		g_ip = inet_ntoa(client_addr.sin_addr);
		g_port = ntohs(client_addr.sin_port);

		pid_t pid;
		if ((pid = fork()) < 0)
        {
            perror("fork error");
            raise(SIGINT);
        }

		if (pid == 0)
		{
			close(server_fd);
			if((fp_checkIP = fopen("access.txt", "r")) == NULL) // open access.txt file and check error
			{
				write_log(log_fd, NULL, 0, DISCONNECT);
				exit(0);
			}
			
			char buf[MAX_BUF]; // store a line from 'access.txt'
			char cli_ip[MAX_BUF];
			strcpy(cli_ip, inet_ntoa(client_addr.sin_addr)); // store client ip to cli_ip

			char *cli_ips[5]; // split client ip address by '.'
			cli_ips[0] = strtok(cli_ip, ".");
			for (int i = 1; (cli_ips[i] = strtok(NULL, ".")) != NULL; i++);
			
			char *ptr, *tmp; // ptr -> check incorrect ip address, tmp -> splitted buf
			while ((ptr = fgets(buf, MAX_BUF, fp_checkIP)) != NULL)
			{
				tmp = strtok(buf, ".\n");
				if (!tmp || (strcmp(tmp, cli_ips[0]) && strcmp(tmp, "*"))) // first Byte dismatch
					continue;
				tmp = strtok(NULL, ".\n");
				if (!tmp || (strcmp(tmp, cli_ips[1]) && strcmp(tmp, "*"))) // second Byte dismatch
					continue;
				tmp = strtok(NULL, ".\n");
				if (!tmp || (strcmp(tmp, cli_ips[2]) && strcmp(tmp, "*"))) // third Byte dismatch
					continue;
				tmp = strtok(NULL, ".\n");
				if (!tmp || (strcmp(tmp, cli_ips[3]) && strcmp(tmp, "*"))) // last Byte dismatch
					continue;
				break; // if match with ip in 'access.txt', stop compare
			}
			fclose(fp_checkIP); // no longer use
			if (!ptr) // escape while() because of no matching
			{
				strcpy(send_buff, "431 This client can't access. Close the session.\n");
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
				close(client_fd);
				write_log(log_fd, NULL, 0, ILLEGAL);
				exit(0);
			}
			else // match
			{
				if ((fp_motd = fopen("motd", "r")) == NULL)
				{
					write_log(log_fd, NULL, 0, DISCONNECT);
					exit(0);
				}
				if (fgets(tmp_buff, TMP_SIZE, fp_motd) == NULL)
				{
					write_log(log_fd, NULL, 0, DISCONNECT);
					exit(0);
				}
				time_t t = time(NULL);
				char tmp[256];
				strftime(tmp, 256, "%a %b %d %X %Z %Y", localtime(&t));
				sprintf(send_buff, tmp_buff, tmp);
				strcpy(tmp_buff, send_buff);
				strcpy(send_buff, "220 ");
				strcat(send_buff, tmp_buff);
				strcat(send_buff, " ready.\n");
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
				fclose(fp_motd);
			}

			if (log_auth(client_fd) == 0)
			{
				close(client_fd);
				exit(0);
			}

			/////////// read FTP command & send result via data connection ///////////
			while (1)
			{
				//////// receive PORT or QUIT ///////////
				if ((n = read(client_fd, buff, BUF_SIZE)) <= 0)
				{
					write_log(log_fd, NULL, 0, DISCONNECT);
					exit(0);
				}
				buff[n] = '\0';
				write(STDOUT_FILENO, buff, strlen(buff));
				write(STDOUT_FILENO, "\n", 1);
				write_log(log_fd, buff, 0, FTP);

				//////// if received QUIT, send '221 Goodbye' and close connection ///////
				if (!strcmp(buff, "QUIT"))
				{
					strcpy(send_buff, "221 Goodbye.\n");
					if (write(client_fd, send_buff, strlen(send_buff)) < 0)
					{
						write_log(log_fd, NULL, 0, DISCONNECT);
						exit(0);
					}
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					close(client_fd);
					write_log(log_fd, NULL, 0, DISCONNECT);
					exit(0);
				}
				else if (!strncmp(buff, "PWD", 3))
				{
					memset(tmp_buff, 0, TMP_SIZE);
					if (PWD(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't print working directory.\n", tmp_buff);
					else
						sprintf(send_buff, "257 \"%s\" is current directory.\n", tmp_buff);
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}
				else if (!strncmp(buff, "CWD", 3))
				{
					memset(tmp_buff, 0, TMP_SIZE);
					if (CWD(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't find such file or directory.\n", tmp_buff);
					else
						strcpy(send_buff, "250 CWD command succeeds.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}
				else if (!strncmp(buff, "CDUP", 4))
				{
					memset(tmp_buff, 0, TMP_SIZE);
					if (CDUP(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't find such file or directory.\n", tmp_buff);
					else
						strcpy(send_buff, "250 CDUP command succeeds.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}
				else if (!strncmp(buff, "DELE", 4))
				{
					memset(tmp_buff, 0, TMP_SIZE);
					if (DELE(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't find such file or directory.\n", tmp_buff);
					else
						strcpy(send_buff, "250 DELE command performed successfully.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}
				else if (!strncmp(buff, "RNFR", 4))
				{
					if (RNFR(buff, tmp_buff) < 0)
					{
						sprintf(send_buff, "550 %s: Can't find such file or directory.\n", tmp_buff);
						write(client_fd, send_buff, strlen(send_buff));
						write(STDOUT_FILENO, send_buff, strlen(send_buff));
						write_log(log_fd, send_buff, 0, RESULT);
						continue;
					}
					strcpy(send_buff, "350 File exists, ready to rename.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);

					if ((n = read(client_fd, buff, BUF_SIZE)) <= 0)
					{
						write_log(log_fd, NULL, 0, DISCONNECT);
						exit(0);
					}
					buff[n] = '\0';
					write(STDOUT_FILENO, buff, strlen(buff));
					write(STDOUT_FILENO, "\n", 1);
					write_log(log_fd, buff, 0, FTP);

					if (RNTO(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't be renamed.\n", tmp_buff);
					else
						strcpy(send_buff, "250 RNTO command succeeds.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}
				else if (!strncmp(buff, "MKD", 3))
				{
					memset(tmp_buff, 0, TMP_SIZE);
					if (MKD(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't create directory.\n", tmp_buff);
					else
						strcpy(send_buff, "250 MKD command performed successfully.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}
				else if (!strncmp(buff, "RMD", 3))
				{
					memset(tmp_buff, 0, TMP_SIZE);
					if (RMD(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't remove directory.\n", tmp_buff);
					else
						strcpy(send_buff, "250 RMD command performed successfully.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}
				else if (!strncmp(buff, "TYPE", 4))
				{
					if (buff[5] == 'I')
					{
						g_mode = 'b';
						strcpy(send_buff, "201 Type set to I.\n");
						
					}
					else if (buff[5] == 'A')
					{
						g_mode = 'a';
						strcpy(send_buff, "201 Type set to A.\n");
					}
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					continue;
				}

				///// if received PORT command, parse it into ip address & port num /////
				convert_str_to_addr(buff, &client_addr);

				////// make data connection socket and connect //////
				data_fd = socket(AF_INET, SOCK_STREAM, 0);
				if (connect(data_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
				{
					strcpy(send_buff, "550 Failed to access.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write_log(log_fd, send_buff, 0, RESULT);
					close(data_fd);
					close(client_fd);
					write_log(log_fd, NULL, 0, DISCONNECT);
					exit(0);
				}

				////// send the success message to client //////
				strcpy(send_buff, "200 PORT command successful\n");
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
				write_log(log_fd, send_buff, 0, RESULT);
				
				///////// read NLST or LIST or RETR or STOR from client ////////////
				if ((n = read(client_fd, buff, BUF_SIZE)) < 0)
				{
					write_log(log_fd, NULL, 0, DISCONNECT);
					exit(0);
				}
				buff[n] = '\0';
				write(STDOUT_FILENO, buff, strlen(buff));
				write(STDOUT_FILENO, "\n", 1);
				write_log(log_fd, buff, 0, FTP);

				////// send client that server will send FTP result via data connection ///////
				if (!strncmp(buff, "NLST", 4) || !strncmp(buff, "LIST", 4))
					strcpy(send_buff, "150 Opening data connection for directory list\n");
				if (!strncmp(buff, "RETR", 4) || !strncmp(buff, "STOR", 4))
				{
					if (g_mode == 'b')
						sprintf(send_buff, "150 Opening binary mode data connection for %s.\n", buff + 5);
					else
						sprintf(send_buff, "150 Opening ascii mode data connection for %s.\n", buff + 5);
				}
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
				write_log(log_fd, send_buff, 0, RESULT);

				if (!strncmp(buff, "RETR", 4) || !strncmp(buff, "STOR", 4))
				{
					char file_name[BUF_SIZE];
					char cmd[5];
					struct stat infor;

					strncpy(cmd, buff, 4);
					cmd[4] = '\0';
					strcpy(file_name, buff + 5);
					if ((stat(file_name, &infor) == 0 && !strncmp(buff, "RETR", 4))
						|| (stat(file_name, &infor) == -1 && !strncmp(buff, "STOR", 4)))
						write(client_fd, file_name, strlen(file_name));
					else
					{
						strcpy(send_buff, "550 Failed transmission.\n");
						write(client_fd, send_buff, strlen(send_buff));
						write(STDOUT_FILENO, send_buff, strlen(send_buff));
						write_log(log_fd, send_buff, 0, RESULT);
						close(data_fd);
						continue;
					}

					if ((n = read(client_fd, buff, BUF_SIZE)) < 0)
					{
						write_log(log_fd, NULL, 0, DISCONNECT);
						exit(0);
					}
					buff[n] = '\0';
					if (!strcmp(buff, "NO"))
					{
						strcpy(send_buff, "550 Failed transmission.\n");
						write(client_fd, send_buff, strlen(send_buff));
						write(STDOUT_FILENO, send_buff, strlen(send_buff));
						write_log(log_fd, send_buff, 0, RESULT);
						close(data_fd);
						continue;
					}
					else
						write(client_fd, "OK", 2);
					if (!strcmp(cmd, "RETR"))
					{
						int file_fd = open(file_name, O_RDONLY);
						n = read(file_fd, send_buff, BUF_SIZE);
						send_buff[n] = '\0';
						close(file_fd);

						if ((n = write(data_fd, send_buff, strlen(send_buff))) < 0)
						{
							perror("write error");
							exit(1);
						}
					}
					else
					{
						if ((n = read(data_fd, buff, BUF_SIZE)) < 0)
						{
							perror("read error");
							exit(1);
						}
						int file_fd = open(file_name, O_WRONLY | O_CREAT, ASCII_MODE);
						write(file_fd, buff, strlen(buff));
						close(file_fd);
					}
					strcpy(send_buff, "226 Complete transmission.");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, "\n", 1);
					write_log(log_fd, send_buff, n, BYTE_RESULT);
					close(data_fd);
					continue;
				}
				else
				{
					////// send FTP command result to client //////
					memset(send_buff, 0, BUF_SIZE);
					if (!strncmp(buff, "NLST", 4))
						n = NLST(buff, send_buff);
					else if (!strncmp(buff, "LIST", 4))
						n = LIST(buff, send_buff);
					if (n < 0)
					{
						write(data_fd, "", 0);
						strcpy(send_buff, "550 Failed transmission.\n"); // if failed, send Fail code
					}
					else if ((n = write(data_fd, send_buff, strlen(send_buff))) < 0)
						strcpy(send_buff, "550 Failed transmission.\n"); // if failed, send Fail code
					else
						strcpy(send_buff, "226 Complete transmission."); // if succeed, send Success code
				}
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, "\n", 1);
				if (!strncmp(send_buff, "226", 3))
					write_log(log_fd, send_buff, n, BYTE_RESULT);
				else
					write_log(log_fd, send_buff, 0, RESULT);
				close(data_fd);
			}
		}
		else
			close(client_fd);
	}
}

////////////////////////////////////////////////////////////////////////
// sh_int                                                             //
// ================================================================== //
// Input: int -> sig                                                  //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: called when SIGINT occured                                //
////////////////////////////////////////////////////////////////////////

void sh_int(int sig)
{
	while (wait(NULL) != -1) // if all of child terminated, parent terminate
	{
		write_log(log_fd, NULL, 0, TERM);
		close(log_fd);
	}
	exit(0);
}

////////////////////////////////////////////////////////////////////////
// convert_str_to_addr                                                //
// ================================================================== //
// Input: char * -> PORT *,*,*,*,*,*                                  //
//        struct sockaddr_in * -> buf for string ip addr, port        //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: convert PORT command to ip addr, port                     //
////////////////////////////////////////////////////////////////////////

void convert_str_to_addr(char *str, struct sockaddr_in *addr)
{
    unsigned long	ip = 0;
    unsigned int	port = 0;
	char			*ptr, *tmp;

	//////// split PORT, ip+port /////////
	strtok(str, " ");
    tmp = strtok(NULL, " ");

	//// make ip address string to 32-bit ip address ////
	ptr = strtok(tmp, ",");
	for (int i = 0; ptr && i < 4; i++)
	{
		ip += atoi(ptr) << (24 - 8*i);
		ptr = strtok(NULL, ",");
	}

	//// combine upper byte #port, lower byte #port into 2 bytes #port ////
	for (int i = 0; ptr && i < 2; i++)
	{
		port += atoi(ptr) << (8 - 8*i);
    	ptr = strtok(NULL, ",");
	}

	////// fill struct sockaddr with ip, port //////
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_addr.s_addr = htonl(ip);
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
}

////////////////////////////////////////////////////////////////////////
// log_auth                                                           //
// ================================================================== //
// Input: int -> socket descriptor that is used for client connection //
//                                                                    //
// Output: int -> 0: failed to login                                  //
//                1: success to login                                 //
// Purpose: compare user name(ID), passwd read from client with       //
//	        ID & passwd in the file 'passwd'                          //
////////////////////////////////////////////////////////////////////////

int log_auth(int connfd)
{
	char buff[BUF_SIZE];
    char user[TMP_SIZE], passwd[TMP_SIZE];
    int n, count = 1;

    while (1)
    {
		if ((n = read(connfd, user, TMP_SIZE)) <= 0)
		{
			write_log(log_fd, NULL, 0, ILLEGAL);
			return 0;
		}
		user[n] = '\0';
		write(STDOUT_FILENO, user, strlen(user));
		write(STDOUT_FILENO, "\n", 1);

		if (user_match(user + 5, NULL) < 0)
		{
			if (count >= 3)
			{
				strcpy(buff, "530 Failed to log-in.\n");
				write(connfd, buff, strlen(buff));
				write(STDOUT_FILENO, buff, strlen(buff));
				strcpy(g_user, user);
				write_log(log_fd, NULL, 0, ILLEGAL);
				return 0;
			}
			strcpy(buff, "430 Invalid username or password.\n");
			write(connfd, buff, strlen(buff));
			write(STDOUT_FILENO, buff, strlen(buff));
			count++;
			continue;
		}
		else
		{
			strcpy(buff, "331 Password is required for username.\n");
			write(connfd, buff, strlen(buff));
			write(STDOUT_FILENO, buff, strlen(buff));
		}

		if ((n = read(connfd, passwd, TMP_SIZE)) <= 0)
			return 0;
		passwd[n] = '\0'; // read user name and passwd from client
		write(STDOUT_FILENO, passwd, strlen(passwd));
		write(STDOUT_FILENO, "\n", 1);

		if (user_match(user + 5, passwd + 5) < 0)
		{
			if (count >= 3)
			{
				strcpy(buff, "530 Failed to log-in.\n");
				write(connfd, buff, strlen(buff));
				write(STDOUT_FILENO, buff, strlen(buff));
				strcpy(g_user, user);
				write_log(log_fd, NULL, 0, ILLEGAL);
				return 0;
			}
			strcpy(buff, "430 Invalid username or password.\n");
			write(connfd, buff, strlen(buff));
			write(STDOUT_FILENO, buff, strlen(buff));
			count++;
			continue;
		}
		else
		{
			sprintf(buff, "230 User %s logged in.\n", user + 5);
			write(connfd, buff, strlen(buff));
			write(STDOUT_FILENO, buff, strlen(buff));
			strcpy(g_user, user + 5);
			g_time = time(NULL);
			write_log(log_fd, NULL, 0, AUTH);
			break;
		}
    }
    return 1; // return success
}

////////////////////////////////////////////////////////////////////////
// user_match                                                         //
// ================================================================== //
// Input: char * -> user name                                         //
//		  char * -> passwd                                            //
//                                                                    //
// Output: int -> 0 : ID exist in 'passwd'                            //
//                1 : ID & passwd exist in 'passwd'                   //
//               -1 : NO ID & passwd in 'passwd'                      //
// Purpose: compare ID & passwd(parameter) with ID & passwd in passwd //                       
////////////////////////////////////////////////////////////////////////

int user_match(char *user, char *passwd)
{
    FILE *fp;
    struct passwd *pw;

    fp = fopen("passwd", "r"); // open passwd file stream for read

	while ((pw = fgetpwent(fp)) != NULL) // parsing passwd line(ID:passwd:uid:gid:...) and store to struct passwd
	{
		if (!strcmp(user, pw->pw_name) && (passwd == NULL))
		{
			fclose(fp);
			return 0;
		}

		if (!strcmp(user, pw->pw_name) && !strcmp(passwd, pw->pw_passwd)) // correspond with passwd
		{
			fclose(fp);
			return 1; // login success
		}
	}
	fclose(fp); // close file stream
	return -1;
}

////////////////////////////////////////////////////////////////////////
// MtoS                                                               //
// ================================================================== //
// Input: struct stat * -> file stat structure                        //
//        const char * -> file path name                              //
//        char * -> buf for file information string                   //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: change file stat structure to long string format(in ls -l)//
////////////////////////////////////////////////////////////////////////

void	MtoS(struct stat *infor, const char *pathname, char *print_buf)
{
	char	time_buf[32];
	char	str[11];

	////// determine file type //////
	if (S_ISREG(infor->st_mode))
		str[0] = '-';
	else if (S_ISDIR(infor->st_mode))
		str[0] = 'd';
	else if (S_ISCHR(infor->st_mode))
		str[0] = 'c';
	else if (S_ISBLK(infor->st_mode))
		str[0] = 'b';
	else if (S_ISFIFO(infor->st_mode))
		str[0] = 'p';
	else if (S_ISLNK(infor->st_mode))
		str[0] = 'l';
	else if (S_ISSOCK(infor->st_mode))
		str[0] = 's';
	
	////// determine user's permission r: read, w: write, x: execute //////
	str[1] = (infor->st_mode & S_IRUSR) ? 'r' : '-';
    str[2] = (infor->st_mode & S_IWUSR) ? 'w' : '-';
    str[3] = (infor->st_mode & S_IXUSR) ? 'x' : '-';
	////// determine group's permission r: read, w: write, x: execute //////
    str[4] = (infor->st_mode & S_IRGRP) ? 'r' : '-';
    str[5] = (infor->st_mode & S_IWGRP) ? 'w' : '-';
    str[6] = (infor->st_mode & S_IXGRP) ? 'x' : '-';
	////// determine other's permission r: read, w: write, x: execute //////
    str[7] = (infor->st_mode & S_IROTH) ? 'r' : '-';
    str[8] = (infor->st_mode & S_IWOTH) ? 'w' : '-';
    str[9] = (infor->st_mode & S_IXOTH) ? 'x' : '-';
	////// null-terminated //////
    str[10] = '\0';

	////// change time_t to formatted string time, stored in time_buf //////
	strftime(time_buf, sizeof(time_buf), "%b %d %R", localtime(&(infor->st_mtime)));

	////// if the infor's file is directory, append '/' next to the file name //////
	if (S_ISDIR(infor->st_mode))
	{
		////// write formatted file informantion string to buf using sprintf() ////// 
		sprintf(print_buf, "%s %2ld %s %s %6ld %s %s/\n",
			str, infor->st_nlink, getpwuid(infor->st_uid)->pw_name,
			getgrgid(infor->st_gid)->gr_name, infor->st_size,
			time_buf, pathname);
	}
	else
	{
		sprintf(print_buf, "%s %2ld %s %s %6ld %s %s\n",
			str, infor->st_nlink, getpwuid(infor->st_uid)->pw_name,
			getgrgid(infor->st_gid)->gr_name, infor->st_size,
			time_buf, pathname);
	}
}

////////////////////////////////////////////////////////////////////////
// NLST                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int : 0 -> success, -1 -> fail                             //
//                                                                    //
// Purpose: execute NLST command(in cli: ls)                          //
////////////////////////////////////////////////////////////////////////

int		NLST(char *buf, char *print_buf)
{
	char 			c;
	int				idx;
	int				start_idx;
	int				len = 0;
	int				aflag = 0;
	int				lflag = 0;

	char			*tmp;

	DIR				*dp;
	struct dirent	*dirp;
	struct stat		infor;

	char			*split[256];
	char			*filename[256];
	char			pathname[256];
	char			tmp_buf[MAX_BUF];
	char			path_buf[MAX_BUF];
	char			line_buf[MAX_BUF];
	optind = 0;
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////////// option parsing -> if unknown option -> error ///////////
	while ((c = getopt(len, split, "al")) != -1)
	{
		switch (c)
		{
		case 'a':
			aflag++;
			break;
		case 'l':
			lflag++;
			break;
		case '?':
			return -1;
		}
	}
	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
		return -1;

	memset(pathname, 0, 256);

	/////////// set pathname, if argument is none, '.'(current dir) is pathname ////////////
	if (optind == len)
		strcpy(pathname, ".");
	else if (split[optind][0] == '~')
	{
		strcpy(pathname, getenv("HOME"));
		if (strlen(split[optind]) > 1)
			strcat(pathname, split[optind] + 1);
	}
	else
		strcpy(pathname, split[optind]);
	////////////////////// open directory stream by pathname ////////////////////////////
	/////////// if pathname is not a directory or has other problem, dp = NULL //////////
	if ((dp = opendir(pathname)) == NULL)
	{
		////// if pathname is not a directory and command has only -l option, not error //////
		if (errno == ENOTDIR)
		{
			if (lflag)
			{
				//////// load file status in struct stat infor ////////
				if (stat(pathname, &infor) == -1)
					return -1;
				////////// print formatted file status string and exit //////////
				MtoS(&infor, pathname, print_buf);
			}
			else
				strcpy(print_buf, pathname);
				strcat(print_buf, "\n");
			return 0;
		}
		return -1;
	}

	////////////// if succeed to open directory stream, read directory entries and store in filename[] ///////////
	idx = 0;
	while (dirp = readdir(dp)) // read until meet NULL -> means read all file names in the directory //
	{
		if (dirp->d_ino != 0)
			filename[idx++] = dirp->d_name;
	}
	

	//////////// sort filenames by ascii code ///////////
	len = idx;
	for (int i = 0; i < len - 1; i++)
	{
		for (int j = i + 1; j < len; j++)
		{
			if (strcmp(filename[i], filename[j]) > 0)
			{
				tmp = filename[i];
				filename[i] = filename[j];
				filename[j] = tmp;
			}
		}
	}

	///////////// find '.', '..', and hidden file index -> to implement no -a option //////////////
	idx = len;
	for (int i = optind; i < len; i++)
	{
		if (filename[i][0] != '.')
		{
			idx = i;
			break;
		}
	}
	
	////////////// if '-a' option exists, print hidden files ////////////
	if (aflag)
		start_idx = 0;
	else
		start_idx = idx;
	
	///////////// if '-l' option exist, print file status information //////////
	if (lflag)
	{
		////// print all file status information for directory entries ///////
		for (int i = start_idx; i < len; i++)
		{
			memset(path_buf, 0, sizeof(path_buf));
			strcat(path_buf, pathname);
			strcat(path_buf, "/");
			strcat(path_buf, filename[i]);
			if (stat(path_buf, &infor) == -1)
				return -1;
			MtoS(&infor, filename[i], line_buf);
			if (strlen(line_buf) + strlen(print_buf) < BUF_SIZE)
				strcat(print_buf, line_buf);
		}
	}
	else ////// if no '-l' option, just print file names //////
	{
		for (int i = start_idx; i < len; i++)
		{
			strcat(print_buf, filename[i]);
			memset(path_buf, 0, sizeof(path_buf));
			strcat(path_buf, pathname);
			strcat(path_buf, "/");
			strcat(path_buf, filename[i]);
			if (stat(path_buf, &infor) == -1)
				return -1;
			if (S_ISDIR(infor.st_mode)) // if the file is directory, write '/' behind its name
				strcat(print_buf, "/");
			strcat(print_buf, "\n");
		}
	}
	closedir(dp);
}

////////////////////////////////////////////////////////////////////////
// LIST                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute LIST command(in cli: dir == ls -al)               //
////////////////////////////////////////////////////////////////////////

int		LIST(char *buf, char *print_buf)
{
	int				idx;
	int				len = 0;
	int				aflag = 0;
	int				lflag = 0;

	char			*errorM;
	char			*tmp;

	DIR				*dp;
	struct dirent	*dirp;
	struct stat		infor;


	char			*split[256];
	char			*filename[256];

	char			pathname[256];
	char			tmp_buf[MAX_BUF];
	char			path_buf[MAX_BUF];
	char			line_buf[MAX_BUF];
	optind = 0;
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
		return -1;

	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
		return -1;

	memset(pathname, 0, 256);

	/////////// set pathname, if argument is none, '.'(current dir) is pathname ////////////
	if (optind == len)
		strcpy(pathname, ".");
	else if (split[optind][0] == '~')
	{
		strcpy(pathname, getenv("HOME"));
		if (strlen(split[optind]) > 1)
			strcat(pathname, split[optind] + 1);
	}
	else
		strcpy(pathname, split[optind]);
	
	////////////////////// open directory stream by pathname ////////////////////////////
	if ((dp = opendir(pathname)) == NULL)
	{
		///// if pathname is not a directory and command has only -l option, not error //////
		if (errno == ENOTDIR)
		{
			//////// load file status in struct stat infor ////////
			if (stat(pathname, &infor) == -1)
				return -1;
			////////// print formatted file status string and exit //////////
			MtoS(&infor, pathname, print_buf);
			return 0;
		}
		return -1;
	}

	////////////// if succeed to open directory stream, read directory entries and store in filename[] ///////////
	idx = 0;
	while (dirp = readdir(dp)) // read until meet NULL -> means read all directory entries
	{
		if (dirp->d_ino != 0)
			filename[idx++] = dirp->d_name;
	}
	
	//////////// sort filenames by ascii code ///////////
	len = idx;
	for (int i = 0; i < len - 1; i++)
	{
		for (int j = i + 1; j < len; j++)
		{
			if (strcmp(filename[i], filename[j]) > 0)
			{
				tmp = filename[i];
				filename[i] = filename[j];
				filename[j] = tmp;
			}
		}
	}
	//////////// same as ls -la, print file stat infor//////////////////
	for (int i = 0; i < len; i++)
	{
		memset(path_buf, 0, sizeof(path_buf));
		strcat(path_buf, pathname);
		strcat(path_buf, "/");
		strcat(path_buf, filename[i]);
		if (stat(path_buf, &infor) == -1)
			return -1;
		MtoS(&infor, filename[i], line_buf);
		if (strlen(line_buf) + strlen(print_buf) < BUF_SIZE)
			strcat(print_buf, line_buf);
	}
	closedir(dp);
}

////////////////////////////////////////////////////////////////////////
// PWD                                                                //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute PWD command(in cli: pwd)                          //
////////////////////////////////////////////////////////////////////////

int		PWD(char *buf, char *print_buf)
{
	int		len = 0;

	char	*split[256];
	char	path_buf[257];
	char	tmp_buf[MAX_BUF];

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}
	split[len] = NULL;

	//////////// if # of arguments are not zero -> error ////////////
	if (len > 1)
		return -1;

	//////////// get current working directory in path_buf ///////////
	getcwd(path_buf, sizeof(path_buf));

	//////////// print current working directory ////////////
	strcpy(print_buf, path_buf);
	return 0;
}

////////////////////////////////////////////////////////////////////////
// CWD                                                                //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute CWD command(in cli: cd)                           //
////////////////////////////////////////////////////////////////////////

int		CWD(char *buf, char *print_buf)
{
	int		len = 0;

	char	pathname[256];
	char	*split[256];
	char	path_buf[257];
	char	tmp_buf[MAX_BUF];

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
		return -1;

	//////////// if # of arguments are more than one -> error ////////////
	if (len > 2)
		return -1;

	///// change '~' to home directory /////
	if (split[1][0] == '~')
	{
		strcpy(pathname, getenv("HOME"));
		if (strlen(split[1]) > 1)
			strcat(pathname, split[1] + 1);
	}
	else
		strcpy(pathname, split[1]);

	///////////// change working directory to argv[1]. if failed, print error and exit //////////////
	if (chdir(pathname)< 0)
	{
		strcpy(print_buf, pathname);
		return -1;
	}
	
	return 0;
}

////////////////////////////////////////////////////////////////////////
// CDUP                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute CDUP command(in cli: cd ..)                       //
////////////////////////////////////////////////////////////////////////

int		CDUP(char *buf, char *print_buf)
{
	int		len = 0;

	char	*split[256];
	char	path_buf[257];
	char	tmp_buf[MAX_BUF];

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}

	//////////// if # of arguments are not zero -> error ////////////
	if (len > 1)
		return -1;

	////////////// change directory to parent directory. if failed, print error message ///////////////
	if (chdir("..")< 0)
		return -1;
	
	return 0;
}

////////////////////////////////////////////////////////////////////////
// MKD                                                                //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute MKD command(in cli: mkdir                         //
////////////////////////////////////////////////////////////////////////

int		MKD(char *buf, char *print_buf)
{
	int		len = 0;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	line_buf[MAX_BUF];
	
	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}

	//////////// if # of arguments are not one -> error ////////////
	if (len != 2)
		return -1;

	////// make directory(filename, permission) //////
	if (mkdir(split[1], 0700) == -1)
	{
		strcpy(print_buf, split[1]);
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
// DELE                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute DELE command(in cli: delete)                      //
////////////////////////////////////////////////////////////////////////

int		DELE(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	line_buf[MAX_BUF];

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}

	//////////// if # of arguments are not one -> error ////////////
	if (len != 2)
		return -1;

	/////// unlink()-> remove file //////////
	if (unlink(split[1]) == -1)
	{
		strcpy(print_buf, split[1]);
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
// RMD                                                                //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute RMD command(in cli: rmdir)                        //
////////////////////////////////////////////////////////////////////////

int		RMD(char *buf, char *print_buf)
{
	int		len = 0;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	line_buf[MAX_BUF];

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}

	//////////// if # of arguments are not one -> error ////////////
	if (len != 2)
		return -1;

	/////// rmdir() -> remove empty direcoty. if failed -> print error ///////
	if (rmdir(split[1]) == -1)
	{
		strcpy(print_buf, split[1]);
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
// RNFR                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute RNFR command(in cli: rename)                      //
////////////////////////////////////////////////////////////////////////

int		RNFR(char *buf, char *name_from)
{
	int		len = 0;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	struct stat infor;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}

	strcpy(name_from, split[1]);
	////// if file doesn't exist, stat() will return -1  -> rename error //////
	if (stat(split[1], &infor) == -1)
		return -1;

	return 0;
}

////////////////////////////////////////////////////////////////////////
// RNTO                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: int -> 0: success                                          //
//               -1: fail                                             //
// Purpose: execute RNTO command(in cli: rename)                      //
////////////////////////////////////////////////////////////////////////

int		RNTO(char *buf, char *name_from)
{
	int		len = 0;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	struct stat infor;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
	{
		split[len++] = ptr;
		if (ptr[0] == '-')
			return -1;
	}

	////// if file exist, stat() will return 0  -> rename error //////
	if (!stat(split[1], &infor))
		return -1;

	////// if failed to rename, print error and exit //////
	if (rename(name_from, split[1]) == -1)
		return -1;

	return 0;
}

void	write_log(int fd, char *command, int bytes, int type)
{
	time_t	t;
	char	str[TMP_SIZE];
	char	buf[BUF_SIZE];

	memset(buf, 0, BUF_SIZE);
	t = time(NULL);
	strftime(str, TMP_SIZE, "%c", localtime(&t));
	switch (type)
	{
		case START:
			sprintf(buf, "%s Server is started\n\n", str);
			break;
		case ILLEGAL:
			sprintf(buf, "%s [%s:%d] %s LOG_FAIL\n\n", str, g_ip, g_port, g_user);
			break;
		case AUTH:
			sprintf(buf, "%s [%s:%d] %s LOG_IN\n\n", str, g_ip, g_port, g_user);
			break;
		case FTP:
			sprintf(buf, "%s [%s:%d] %s %s\n\n", str, g_ip, g_port, g_user, command);
			break;
		case RESULT:
			sprintf(buf, "%s [%s:%d] %s %s\n", str, g_ip, g_port, g_user, command);
			break;
		case BYTE_RESULT:
			sprintf(buf, "%s [%s:%d] %s %s | %d bytes\n\n", str, g_ip, g_port, g_user, command, bytes);
			break;
		case DISCONNECT:
			sprintf(buf, "%s [%s:%d] %s LOG_OUT\n[total service time : %ld sec]\n\n", str, g_ip, g_port, g_user, time(NULL) - g_time);
			break;
		case TERM:
			sprintf(buf, "%s Server is terminated\n\n", str);
			break;
	}
	write(fd, buf, strlen(buf));
}