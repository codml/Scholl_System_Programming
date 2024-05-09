////////////////////////////////////////////////////////////////////////
// File Name    :srv.c                                                //
// Date         :2024/05/                                             //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #2-3:                  //
// Description  :                                                     //
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

#define MAX_BUF 4096
#define BUF_SIZE 4096
#define PID 0
#define PORT 1
#define START_TIME 2

void    sh_chld(int sig);
void    sh_alrm(int sig);
void    sh_int(int sig);

void	MtoS(struct stat *infor, const char *pathname, char *print_buf);
void	NLST(char *buf, char *print_buf);
void	LIST(char *buf, char *print_buf);
void	PWD(char *buf, char *print_buf);
void	CWD(char *buf, char *print_buf);
void	CDUP(char *buf, char *print_buf);
void	MKD(char *buf, char *print_buf);
void	DELE(char *buf, char *print_buf);
void	RMD(char *buf, char *print_buf);
void	RN(char *buf, char *print_buf);
void	QUIT(char *buf, char *print_buf);

int     client_info(struct sockaddr_in *cliaddr);

int		process[BUF_SIZE][3];
int		process_start = 0;
int		process_end = 0;
int		process_cnt = 0;

int main(int argc, char **argv)
{
    char buff[BUF_SIZE], send_buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd;
    int len;

    ///// check the number of arguments /////
    if (argc != 2)
    {
        write(2, "One argument is needed: port\n", strlen("one argument is needed: port\n"));
        exit(1);
    }

    //// register functions which is called when signal occurred //////
    signal(SIGCHLD, sh_chld);
    signal(SIGALRM, sh_alrm);
    signal(SIGINT, sh_int);

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
            while (1)
            {
                memset(buff, 0, BUF_SIZE);
				memset(send_buff, 0, BUF_SIZE);
                if ((n = read(client_fd, buff, BUF_SIZE) > 0))
                {
                    if (!strncmp(buff, "NLST", 4))
						NLST(buff, send_buff);
					else if (!strncmp(buff, "LIST", 4))
						LIST(buff, send_buff);
					else if (!strncmp(buff, "PWD", 3))
						PWD(buff, send_buff);
					else if (!strncmp(buff, "CWD", 3))
						CWD(buff, send_buff);
					else if (!strncmp(buff, "CDUP", 4))
						CDUP(buff, send_buff);
					else if (!strncmp(buff, "MKD", 3))
						MKD(buff, send_buff);
					else if (!strncmp(buff, "DELE", 4))
						DELE(buff, send_buff);
					else if (!strncmp(buff, "RMD", 3))
						RMD(buff, send_buff);
					else if (!strncmp(buff, "RNFR", 4))
						RN(buff, send_buff);
					else if (!strncmp(buff, "QUIT", 4))
						QUIT(buff, send_buff);
					else
						strcpy(send_buff, "unknown command\n");
                }
				if (!strcmp(send_buff, "QUIT"))
					break;
				write(client_fd, send_buff, strlen(send_buff));
            }
            close(client_fd);
			sprintf(buff, "Client (%5d)'s Release\n\n", getpid());
			write(STDOUT_FILENO, buff, strlen(buff));
            exit(0);
        }
        else
        {
            ////// close connection with client in parent process //////
			process_cnt++;
			if (process_end < BUF_SIZE)
			{
				process[process_end][PID] = pid;
				process[process_end][PORT] = ntohs(client_addr.sin_port);
				process[process_end][START_TIME] = time(NULL);
				process_end++;
			}
            close(client_fd);
			alarm(1);
        }
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
	char	buff[BUF_SIZE];
	pid_t	pid;

	///// when SIGCHLD occurred, this function is called /////
	pid = wait(NULL);
	process_cnt--;
	for (int i = process_start; i < process_end; i++)
	{
		if (process[i][PID] == pid)
		{
			process[i][PID] = 0;
			if (i == process_start)
			{
				process_start++;
				if (process_start == process_end)
					process_start = process_end = 0;
			}
			else if (i == process_end - 1)
				process_end--;
			break;
		}
	}
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
	char	buf[BUF_SIZE];

	alarm(0);
	if (process_cnt == 0)
		return ;

	sprintf(buf, "Current Number of Client : %4d\n", process_cnt);
	write(STDOUT_FILENO, buf, strlen(buf));
	sprintf(buf, "%5s\t%5s\t%4s\n", "PID", "PORT", "TIME");
	write(STDOUT_FILENO, buf, strlen(buf));
	for (int i = process_start; i < process_end; i++)
	{
		if (process[i][PID] == 0)
			continue;
		
		sprintf(buf, "%5d\t%5d\t%4ld\n", process[i][PID], process[i][PORT], time(NULL) - process[i][START_TIME]);
		write(STDOUT_FILENO, buf, strlen(buf));
	}
	alarm(10);
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
	while (wait(NULL) != -1);
	exit(0);
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

	char			*pathname;
	char			*errorM;
	char			*tmp;

	DIR				*dp;
	struct dirent	*dirp;
	struct stat		infor;

	char			*split[256];
	char			*filename[256];
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

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

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
				sprintf(print_buf, "Error : %s\n", strerror(errno));
				return ;
			}
			////////// print formatted file status string and exit //////////
			MtoS(&infor, pathname, print_buf);
			return ;
		}
		///////// if error caused by other problem, print error string and exit ////////
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
			if ((i - start_idx) % 5 == 4)
				strcat(print_buf, "\n");
			else
				strcat(print_buf, " ");
		}
		strcat(print_buf, "\n");
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

	char			*pathname;
	char			*errorM;
	char			*tmp;

	DIR				*dp;
	struct dirent	*dirp;
	struct stat		infor;


	char			*split[256];
	char			*filename[256];

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

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		strcat(print_buf, errorM);
		return ;
	}

	/////////// set pathname, if argument is none, '.'(current dir) is pathname ////////////
	if (optind == len)
		pathname = ".";
	else if (!strcmp(split[optind], "~"))
		pathname = getenv("HOME");
	else
		pathname = split[optind];
	
	////////////////////// open directory stream by pathname ////////////////////////////
	if ((dp = opendir(pathname)) == NULL)
	{
		////// if pathname is not a directory and command has only -l option, not error //////
		if (errno == ENOTDIR)
		{
			if(stat(pathname, &infor) == -1)
			{
				if (errno == EACCES)
					errorM = "cannot access";
				else if (errno == ENOENT)
					errorM = "No such file or directory";
				else
					errorM = strerror(errno);
				sprintf(print_buf, "Error : %s\n", errorM);
				return ;
			}
			MtoS(&infor, pathname, print_buf);
			return ;
		}
		///////// if error caused by other problem, print error string and exit ////////
		sprintf(print_buf, "Error : %s\n", strerror(errno));
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
// Output: None                                                       //
//                                                                    //
// Purpose: execute PWD command(in cli: pwd)                          //
////////////////////////////////////////////////////////////////////////

void	PWD(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	char	tmp_buf[MAX_BUF];

	optind = 0;
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are not zero -> error ////////////
	if (len > 1)
	{
		errorM = "Error: argument is not required\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// get current working directory in path_buf ///////////
	getcwd(path_buf, sizeof(path_buf));

	//////////// print current working directory ////////////
	sprintf(print_buf, "\"%s\" is current directory\n", path_buf);
}

////////////////////////////////////////////////////////////////////////
// CWD                                                                //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute CWD command(in cli: cd)                           //
////////////////////////////////////////////////////////////////////////

void	CWD(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*pathname;
	char	*split[256];
	char	path_buf[257];
	char	tmp_buf[MAX_BUF];
	optind = 0;
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are more than one -> error ////////////
	if (len > 2)
	{
		errorM = "Error: too much argument\n";
		strcat(print_buf, errorM);
		return ;
	}

	///// change '~' to home directory /////
	if (!strcmp(split[1], "~"))
		pathname = getenv("HOME");
	else
		pathname = split[1];

	///////////// change working directory to argv[1]. if failed, print error and exit //////////////
	if (chdir(pathname)< 0)
	{
		if (errno == ENOENT)
			errorM = "directory not found";
		else
			errorM = strerror(errno);
		sprintf(print_buf, "Error: %s\n", errorM);
		return ;
	}
	
	/////////// to check the result, get current working directory ///////////////
	getcwd(path_buf, sizeof(path_buf));

	///////// print current working directory //////////
	sprintf(print_buf, "\"%s\" is current directory\n", path_buf);
}

////////////////////////////////////////////////////////////////////////
// CDUP                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute CDUP command(in cli: cd ..)                       //
////////////////////////////////////////////////////////////////////////

void	CDUP(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	optind = 0;
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are not zero -> error ////////////
	if (len > 1)
	{
		errorM = "Error: too much argument\n";
		strcat(print_buf, errorM);
		return ;
	}

	////////////// change directory to parent directory. if failed, print error message ///////////////
	if (chdir("..")< 0)
	{
		if (errno == ENOENT)
			errorM = "directory not found";
		else
			errorM = strerror(errno);
		sprintf(print_buf, "Error: %s\n", errorM);
		return ;
	}
	
	/////////// to check the result, get current working directory ///////////////
	getcwd(path_buf, sizeof(path_buf));
	
	///////// print current working directory //////////
	sprintf(print_buf, "\"%s\" is current directory\n", path_buf);
}

////////////////////////////////////////////////////////////////////////
// MKD                                                                //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute MKD command(in cli: mkdir                         //
////////////////////////////////////////////////////////////////////////

void	MKD(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	line_buf[MAX_BUF];
	optind = 0;
	opterr = 0;
	
	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		strcat(print_buf, errorM);
		return ;
	}

	/////////// try to make directory by arguments(file_name)
	for (int i = optind; i < len; i++)
	{
		////// make directory(filename, permission) //////
		if (mkdir(split[i], 0700) == -1)
		{
			////////// if failed to make, print error /////////
			if (errno == EEXIST)
				errorM = "File exists";
			else
				errorM = strerror(errno);
			sprintf(line_buf, "Error: cannot create directory \'%s\': %s\n", split[i], errorM);
			strcat(print_buf, line_buf);
		}
		else
		{
			/// print command, file name ///
			sprintf(line_buf, "%s %s\n", split[0], split[i]);
			strcat(print_buf, line_buf);
		}
	}
}

////////////////////////////////////////////////////////////////////////
// DELE                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute DELE command(in cli: delete)                      //
////////////////////////////////////////////////////////////////////////

void	DELE(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	line_buf[MAX_BUF];
	optind = 0;
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		strcat(print_buf, errorM);
		return ;
	}

	/////////// remove each files that correspond with the arguments ///////////
	for (int i = optind; i < len; i++)
	{
		/////// unlink()-> remove file //////////
		if (unlink(split[i]) == -1)
		{
			////// if failed to remove, print error message //////
			sprintf(line_buf, "Error: failed to delete \'%s\'\n", split[i]);
			strcat(print_buf, line_buf);
		}
		else
		{
			/////// print result ////////
			sprintf(line_buf, "%s %s\n", split[0], split[i]);
			strcat(print_buf, line_buf);
		}
	}
}

////////////////////////////////////////////////////////////////////////
// RMD                                                                //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute RMD command(in cli: rmdir)                        //
////////////////////////////////////////////////////////////////////////

void	RMD(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	line_buf[MAX_BUF];
	optind = 0;
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(tmp_buf, buf);
	for (char *ptr = strtok(tmp_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		strcat(print_buf, errorM);
		return ;
	}

	//////////// remove empty directories that correspond with the arguments //////////
	for (int i = optind; i < len; i++)
	{
		/////// rmdir() -> remove empty direcoty. if failed -> print error ///////
		if (rmdir(split[i]) == -1)
		{
			sprintf(line_buf, "Error: failed to remove \'%s\'\n", split[i]);
			strcat(print_buf, line_buf);
		}
		else
		{
			//////// print result /////////
			sprintf(line_buf, "%s %s\n", split[0], split[i]);
			strcat(print_buf, line_buf);
		}
	}
}

////////////////////////////////////////////////////////////////////////
// RN                                                                 //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute RNFR & RNTO command(in cli: rename)               //
////////////////////////////////////////////////////////////////////////

void	RN(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	struct stat infor;
	optind = 0;
	opterr = 0;

	///////// print command name //////////
	sprintf(print_buf, "> %s\t\t[%d]\n", split[0], getpid());
	write(1, print_buf, strlen(print_buf));
	memset(print_buf, 0, BUF_SIZE);

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
		return ;
	}

	//////////// if # of arguments are not two(old, new) -> error ////////////
	if (len != 4 | strcmp(split[2], "RNTO"))
	{
		errorM = "Error: two argumens are required\n";
		strcat(print_buf, errorM);
		return ;
	}

	////// if file exist, stat() will return 0  -> rename error //////
	if (!stat(split[3], &infor))
	{
		errorM = "Error: name to change already exists\n";
		strcat(print_buf, errorM);
		return ;
	}

	////// if failed to rename, print error and exit //////
	if (rename(split[1], split[3]) == -1)
	{
		sprintf(print_buf, "Error: %s\n", strerror(errno));
		strcat(print_buf, errorM);
		return ;
	}

	////// print result //////
	sprintf(print_buf, "%s %s\n%s %s\n", split[0], split[1], split[2], split[3]);
}

////////////////////////////////////////////////////////////////////////
// QUIT                                                               //
// ================================================================== //
// Input: char * ->  ftp command from cli                             //
//                                                                    //
// Output: None                                                       //
//                                                                    //
// Purpose: execute QUIT command(in cli: quit)                        //
////////////////////////////////////////////////////////////////////////

void	QUIT(char *buf, char *print_buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
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
		return ;
	}

	//////////// if # of arguments are not zero -> error ////////////
	if (len != 1)
	{
		errorM = "Error: argument is not required\n";
		strcat(print_buf, errorM);
		return ;
	}

	////// print result ///////
	strcpy(print_buf, "QUIT");
}
