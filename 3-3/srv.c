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

#define BUF_SIZE 4096
#define TMP_SIZE 1024
#define MAX_BUF 4096

void	convert_str_to_addr(char *str, struct sockaddr_in *addr);
int		log_auth(int connfd);
int		user_match(char *user, char *passwd);
void	NLST(char *buf, char *print_buf);
void	MtoS(struct stat *infor, const char *pathname, char *print_buf);
void	LIST(char *buf, char *print_buf);
int		PWD(char *buf, char *print_buf);
int		CWD(char *buf, char *print_buf);
int		CDUP(char *buf, char *print_buf);
int		MKD(char *buf, char *print_buf);
int		DELE(char *buf, char *print_buf);
int		RMD(char *buf, char *print_buf);
int		RNFR(char *buf, char *name_from);
int		RNTO(char *buf, char *name_from);
void	RETR(char *buf, char *print_buf);
void	STOR(char *buf, char *print_buf);

void main(int argc, char **argv)
{
    char buff[BUF_SIZE], send_buff[BUF_SIZE], tmp_buff[TMP_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd, data_fd;
    int len;
	FILE *fp_checkIP, *fp_motd;

    ///// check the number of arguments /////
    if (argc != 2)
    {
        write(2, "One argument is needed: port\n", strlen("one argument is needed: port\n"));
        exit(1);
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
        exit(1);
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
			exit(1);
		}

		pid_t pid;
		if ((pid = fork()) < 0)
        {
            perror("fork error");
            exit(1);
        }

		if (pid == 0)
		{
			close(server_fd);
			if((fp_checkIP = fopen("access.txt", "r")) == NULL) // open access.txt file and check error
				exit(1);
			
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
				continue;
			}
			else // match
			{
				if ((fp_motd = fopen("motd", "r")) == NULL)
					raise(SIGINT);
				if (fgets(tmp_buff, TMP_SIZE, fp_motd) == NULL)
					raise(SIGINT);
				time_t t = time(NULL);
				sprintf(send_buff, tmp_buff, ctime(&t));
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
			}

			if (log_auth(client_fd) == 0)
			{
				close(client_fd);
				raise(SIGINT);
			}

			/////////// read FTP command & send result via data connection ///////////
			while (1)
			{
				//////// receive PORT or QUIT ///////////
				if ((n = read(client_fd, buff, BUF_SIZE)) <= 0)
				{
					perror("read error");
					exit(1);
				}
				buff[n] = '\0';
				write(STDOUT_FILENO, buff, strlen(buff));
				write(STDOUT_FILENO, "\n", 1);

				//////// if received QUIT, send '221 Goodbye' and close connection ///////
				if (!strcmp(buff, "QUIT"))
				{
					strcpy(send_buff, "221 Goodbye.\n");
					if (write(client_fd, send_buff, strlen(send_buff)) < 0)
					{
						perror("write error");
						exit(1);
					}
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
					close(client_fd);
					exit(0);;
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
					continue;
				}
				else if (!strncmp(buff, "RNFR", 4))
				{
					if (RNFR(buff, tmp_buff) < 0)
					{
						sprintf(send_buff, "550 %s: Can't find such file or directory.\n", tmp_buff);
						write(client_fd, send_buff, strlen(send_buff));
						write(STDOUT_FILENO, send_buff, strlen(send_buff));
						continue;
					}
					strcpy(send_buff, "350 File exists, ready to rename.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));

					if ((n = read(client_fd, buff, BUF_SIZE)) <= 0)
					{
						perror("read error");
						exit(1);
					}
					buff[n] = '\0';
					write(STDOUT_FILENO, buff, strlen(buff));
					write(STDOUT_FILENO, "\n", 1);

					if (RNTO(buff, tmp_buff) < 0)
						sprintf(send_buff, "550 %s: Can't be renamed.\n", tmp_buff);
					else
						strcpy(send_buff, "250 RNTO command succeeds.\n");
					write(client_fd, send_buff, strlen(send_buff));
					write(STDOUT_FILENO, send_buff, strlen(send_buff));
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
					continue;
				}
				else if (!strncmp(buff, "TYPE", 4))
				{
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
					close(data_fd);
					close(client_fd);
					exit(1);
				}

				////// send the success message to client //////
				strcpy(send_buff, "200 PORT command successful\n");
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
				
				///////// read NLST or LIST or RETR or STOR from client ////////////
				if ((n = read(client_fd, buff, BUF_SIZE)) < 0)
				{
					perror("read error");
					exit(1);
				}
				buff[n] = '\0';
				write(STDOUT_FILENO, buff, strlen(buff));
				write(STDOUT_FILENO, "\n", 1);

				////// send client that server will send FTP result via data connection ///////
				strcpy(send_buff, "150 Opening data connection for directory list\n");
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));

				////// send FTP command result to client //////
				memset(send_buff, 0, BUF_SIZE);
				if (!strncmp(buff, "NLST", 4))
					NLST(buff, send_buff);
				else if (!strncmp(buff, "LIST", 4))
					LIST(buff, send_buff);
				else if (!strncmp(buff, "RETR", 4))
					RETR(buff, send_buff);
				else if (!strncmp(buff, "STOR", 4))
					STOR(buff, send_buff);
				if (write(data_fd, send_buff, strlen(send_buff)) < 0)
					strcpy(send_buff, "550 Failed transmission.\n"); // if failed, send Fail code
				else
					strcpy(send_buff, "226 Result is sent successfully.\n"); // if succeed, send Success code
				write(client_fd, send_buff, strlen(send_buff));
				write(STDOUT_FILENO, send_buff, strlen(send_buff));
				close(data_fd);
			}
		}
		else
			close(client_fd);
	}
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
		if (count > 3)
		{
			strcpy(buff, "530 Failed to log-in.\n");
			write(connfd, buff, strlen(buff));
			write(STDOUT_FILENO, buff, strlen(buff));
			return 0;
		}

		if ((n = read(connfd, user, MAX_BUF)) <= 0)
			return 0;
		user[n] = '\0';

		if (user_match(user + 5, NULL) < 0)
		{
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

		if ((n = read(connfd, passwd + 5, MAX_BUF)) <= 0)
			return 0;
		passwd[n] = '\0'; // read user name and passwd from client

		if (user_match(user + 5, passwd + 5) < 0)
		{
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
		if (!strcmp(user, pw->pw_name) && !strcmp(passwd, pw->pw_passwd)) // correspond with passwd
		{
			fclose(fp);
			return 1; // login success
		}
		if (!strcmp(user, pw->pw_name) && passwd == NULL)
		{
			fclose(fp);
			return 0;
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
// Output: None                                                       //
//                                                                    //
// Purpose: execute NLST command(in cli: ls)                          //
////////////////////////////////////////////////////////////////////////

void	NLST(char *buf, char *print_buf)
{
	char 			c;
	int				idx;
	int				start_idx;
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
			errorM = "Error: invalid option\n";
			strcat(print_buf, errorM);
			return ;
		}
	}
	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		strcat(print_buf, errorM);
		return ;
	}

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
		///////// if error caused, print error string and exit ////////
		if (errno == EACCES)
			errorM = "cannot access";
		else
			errorM = strerror(errno);
		sprintf(print_buf, "Error : %s\n", errorM);
		return ;
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
			{
				sprintf(print_buf, "Error : %s\n", strerror(errno));
				return ;
			}
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
			{
				sprintf(print_buf, "Error : %s\n", strerror(errno));
				return ;
			}
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

void	LIST(char *buf, char *print_buf)
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
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		write(1, print_buf, strlen(print_buf));
		return ;
	}

	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		strcat(print_buf, errorM);
		write(1, print_buf, strlen(print_buf));
		return ;
	}

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
		///////// if error caused, print error string and exit ////////
		sprintf(print_buf, "Error : %s\n", strerror(errno));
		write(1, print_buf, strlen(print_buf));
		return ;
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
		{
			sprintf(print_buf, "Error : %s\n", strerror(errno));
			write(1, print_buf, strlen(print_buf));
			return ;
		}
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

void	RETR(char *buf, char *print_buf)
{

}

void	STOR(char *buf, char *print_buf)
{

}