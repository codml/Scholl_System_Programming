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
#define SEND_BUFF 1024

int		client_info(struct sockaddr_in *cliaddr);
int		NLST(char *buf, char *result_buf);
void	MtoS(struct stat *infor, const char *pathname, char *print_buf);
int		cmd_process(char *buff, char *result_buff);

int main(int argc, char **argv)
{
    struct  sockaddr_in srvaddr, cliaddr;
    int     serverfd, connfd;
	int		clilen;

    char    buff[MAX_BUFF], result_buff[SEND_BUFF];
    int     n;

    if (argc != 2)
	{
		write(2, "Format: ./srv [port num]\n", strlen("Format: ./srv [port num]\n"));
		exit(1);
	}

    if ((serverfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        write(2, "Server: Can't open stream socket\n", strlen("Server: Can't open stream socket\n"));
        exit(1);
    }

	memset(result_buff, 0, sizeof(result_buff));

    memset((char *)&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	srvaddr.sin_port = htons(atoi(argv[1]));

    if (bind(serverfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0)
    {
        write(2, "Server: Can't bind local address\n", strlen("Server: Can't bind local address\n"));
        exit(1);
    }

    listen(serverfd, 5);

    while (1)
    {
        connfd = accept(serverfd, (struct sockaddr *) &cliaddr, &clilen);
        if (client_info(&cliaddr) < 0)
            write(2, "client_info() err!!\n", strlen("client_info() err!!\n"));
        while (1)
        {
            n = read(connfd, buff, MAX_BUFF);
            buff[n] = '0';
            if (cmd_process(buff, result_buff) < 0)
            {
                write(2, "cmd_process() err!\n", strlen("cmd_process() err!\n"));
                break;
            }
            write(connfd, result_buff, strlen(result_buff));
            if (!strcmp(result_buff, "QUIT"))
            {
                write(2, "QUIT\n", strlen("QUIT\n"));
                close(connfd);
                break;
            }
        }
    }
    close(serverfd);
}

int	client_info(struct sockaddr_in *cliaddr)
{
	char print_buf[MAX_BUFF];
	char buf[MAX_BUFF];

	strcpy(print_buf, "==========Client info==========\n");
	sprintf(buf, "client IP: %s\n\nclient port: %d\n", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port));
	strcat(print_buf, buf);
	strcat(print_buf, "===============================\n");
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

int	NLST(char *buf, char *result_buf)
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
	opterr = 0;

	char			*split[256];
	char			*filename[256];
	char			tmp_buf[MAX_BUFF];
	char			path_buf[MAX_BUFF];
	char			print_buf[MAX_BUFF];
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

	/////////// set pathname, if argument is none, '.'(current dir) is pathname ////////////
	if (optind == len)
		pathname = ".";
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
				return -1; /////// error handling when failed to load file status ///////
			////////// print formatted file status string and exit //////////
			MtoS(&infor, pathname, result_buf);
			return 0;
		}
		///////// if error caused by other problem, print error string and exit ////////
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
			MtoS(&infor, filename[i], print_buf);
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
				return -1;
			if (S_ISDIR(infor.st_mode)) // if the file is directory, write '/' behind its name
				strcat(result_buf, "/");
			strcat(result_buf, "\n");
		}
	}
	closedir(dp);
	return 0;
}

int	cmd_process(char *buff, char *result_buff)
{
	if (!strcmp(buff, "QUIT"))
		strcpy(result_buff, buff);
	else
		return (NLST(buff, result_buff));
	return 0;
}