////////////////////////////////////////////////////////////////////////
// File Name    :srv.c                                                //
// Date         :2024/05/02                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #2-1: socket           //
// Description  :socket programming - srv: receive FTP commands and   //
//										   send the result of command //
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define MAX_BUFF 4096
#define SEND_BUFF 16384

int		client_info(struct sockaddr_in *cliaddr);
void	NLST(char *buf, char *result_buf);
void	MtoS(struct stat *infor, const char *pathname, char *print_buf);
void	cmd_process(char *buff, char *result_buff);

int main(int argc, char **argv)
{
    struct  sockaddr_in srvaddr, cliaddr;
    int     serverfd, connfd;
	int		clilen;
	char	*str;

    char    buff[MAX_BUFF], result_buff[SEND_BUFF];
    int     n;

	/////// check # of arguments /////////
    if (argc != 2)
	{
		write(2, "Format: ./srv [port num]\n", strlen("Format: ./srv [port num]\n"));
		exit(1);
	}

	//////// make socket for server ////////
    if ((serverfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        write(2, "Server: Can't open stream socket\n", strlen("Server: Can't open stream socket\n"));
        exit(1);
    }

	///////// set server address(address family, IPv4 address, port number) /////////
    memset((char *)&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(atoi(argv[1]));

	///////// bind socket and server process /////////
    if (bind(serverfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0)
    {
        write(2, "Server: Can't bind local address\n", strlen("Server: Can't bind local address\n"));
        exit(1);
    }

	////// listen client connect, backlog = 10(queue size) //////
	listen(serverfd, 10);

    while (1)
    {
		//////// accept client's connection & save client infor to cliaddr /////////
		clilen = sizeof(cliaddr);
        connfd = accept(serverfd, (struct sockaddr *)&cliaddr, &clilen);

		////// print client information(client IPv4 address, port number) ///////
        if (client_info(&cliaddr) < 0)
            write(2, "client_info() err!!\n", strlen("client_info() err!!\n"));

		////// reset buffers //////
		memset(buff, 0, sizeof(buff));
		memset(result_buff, 0, sizeof(result_buff));

		////// repeat until client send QUIT /////
		while (1)
		{
			////// read FTP commands from client /////
			if ((n = read(connfd, buff, MAX_BUFF)) < 0)
			{
				write(2, "read() error\n", strlen("read() error\n"));
				break;
			}
			buff[n] = '\0';

			////// process command received from client /////
			cmd_process(buff, result_buff);

			///// write result to clien //////
			if (write(connfd, result_buff, strlen(result_buff)) < 0)
			{
				write(2, "write() err\n", strlen("write() err\n"));
				break;
			}

			////// if result was QUIT, close user connection //////
			if (!strcmp(result_buff, "QUIT"))
				break;

			///// reset buffers //////
			memset(buff, 0, sizeof(buff));
			memset(result_buff, 0, sizeof(result_buff));
		}
		////// close user socket /////
		close(connfd);
    }
	//// close server socket ////
    close(serverfd);
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
	char print_buf[MAX_BUFF];
	char buf[MAX_BUFF];

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
		////// write formatted file informantion string to buf using snprintf() ////// 
		snprintf(print_buf, MAX_BUFF, "%s %2ld %s %s %6ld %s %s/\n",
			str, infor->st_nlink, getpwuid(infor->st_uid)->pw_name,
			getgrgid(infor->st_gid)->gr_name, infor->st_size,
			time_buf, pathname);
	}
	else
	{
		snprintf(print_buf, MAX_BUFF, "%s %2ld %s %s %6ld %s %s\n",
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

void	NLST(char *buf, char *result_buf)
{
	char 			c;
	int				idx;
	int				start_idx;
	int				len = 0;
	int				aflag = 0;
	int				lflag = 0;

	char			*pathname;
	char			*errorM;
	char			*tmp;

	DIR				*dp;
	struct dirent	*dirp;
	struct stat		infor;

	char			*split[256];
	char			*filename[256];
	char			tmp_buf[MAX_BUFF];
	char			path_buf[MAX_BUFF];
	char			print_buf[MAX_BUFF];
	
	opterr = 0;
	optind = 0;

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
			strcpy(result_buf, errorM);
			return;
		}
	}

	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		strcpy(result_buf, errorM);
		return;
	}

	/////////// set pathname, if argument is none, '.'(current dir) is pathname ////////////
	if (optind == len)
		pathname = ".";
	else if (!strcmp(split[optind], "~"))
		pathname = getenv("HOME");
	else
		pathname = split[optind];
	
	////////////////////// open directory stream by pathname ////////////////////////////
	/////////// if pathname is not a directory or has other problem, dp = NULL //////////
	if ((dp = opendir(pathname)) == NULL)
	{
		////// if pathname is not a directory and command has only -l option, not error //////
		if (errno == ENOTDIR && lflag)
		{
			//////// load file status in struct stat infor ////////
			if (stat(pathname, &infor) == -1)
			{
				/////// error handling when failed to load file status ///////
				snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
				strcpy(result_buf, print_buf);
				return;
			}
			////////// print formatted file status string and exit //////////
			MtoS(&infor, pathname, print_buf);
			strcpy(result_buf, print_buf);
			return;
		}
		///////// if error caused by other problem, print error string and exit ////////
		if (errno == EACCES)
			errorM = "cannot access";
		else
			errorM = strerror(errno);
		snprintf(print_buf, sizeof(print_buf), "Error : %s\n", errorM);
		strcpy(result_buf, print_buf);
		return;
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
				snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
				strcpy(result_buf, print_buf);
				return;
			}
			MtoS(&infor, filename[i], print_buf);
			if (strlen(result_buf) + strlen(print_buf) < SEND_BUFF)
				strcat(result_buf, print_buf);
		}
	}
	else ////// if no '-l' option, just print file names //////
	{
		for (int i = start_idx; i < len; i++)
		{
			strcat(result_buf, filename[i]);
			memset(path_buf, 0, sizeof(path_buf));
			strcat(path_buf, pathname);
			strcat(path_buf, "/");
			strcat(path_buf, filename[i]);
			if (stat(path_buf, &infor) == -1)
			{
				snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
				write(2, print_buf, strlen(print_buf));
				return ;
			}
			if (S_ISDIR(infor.st_mode)) // if the file is directory, write '/' behind its name
				strcat(result_buf, "/");
			strcat(result_buf, "\n");
		}
	}
	closedir(dp);
}

////////////////////////////////////////////////////////////////////////
// cmd_process                                                        //
// ================================================================== //
// Input: char *-> FTP command                                        //
//        char *-> memory to store command result(or error string)    //
//                                                                    //
//                                                                    //
// Output: none                                                       //
//                                                                    //
//                                                                    //
// Purpose: execute FTP command and store result to result_buff       //
////////////////////////////////////////////////////////////////////////

void	cmd_process(char *buff, char *result_buff)
{
	//////// execute QUIT instruction -> send 'QUIT' to client ///////
	if (!strcmp(buff, "QUIT"))
		strcpy(result_buff, buff);

	/////// execute NLST instruction -> send result(or error) to client ///////
	else if (!strncmp(buff, "NLST", 4))
		NLST(buff, result_buff);

	////// receive wrong command //////
	else
	{
		strcpy(result_buff, "wrong command\n");
		write(1, "WRONG COMMAND\n", strlen("WRONG COMMAND\n"));
		return ;
	}
	////// print command to server /////
	write(1, buff, strlen(buff));
	write(1, "\n", 1);
}