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
#define MAX_BUF 4096

int convert_str_to_addr(char *str, struct sockaddr_in *addr);
void	NLST(char *buf, char *print_buf);
void	MtoS(struct stat *infor, const char *pathname, char *print_buf);

void main(int argc, char **argv)
{
    char buff[BUF_SIZE], send_buff[BUF_SIZE], cmd[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr;
    int server_fd, client_fd, data_fd;
    int len;

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
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    ////// open queue for client connect /////
    listen(server_fd, 5);

    len = sizeof(client_addr);
    /////// server accept client's connect, get client address & port ///////
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
	while (1)
	{
        if ((n = read(client_fd, buff, BUF_SIZE)) < 0)
        {
            perror("read error");
            exit(1);
        }
        buff[n] = '\0';
        printf("%s\n", buff);
		if (convert_str_to_addr(buff, &client_addr) < 0)
        {
            perror("Read error");
            exit(1);
        }
        data_fd = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(data_fd, (struct sockaddr *)&client_addr, len) < 0)
        {
            perror("connect error");
            exit(1);
        }
        strcpy(send_buff, "200 PORT command successful");
        write(client_fd, send_buff, strlen(send_buff));
        printf("%s\n", send_buff);
        if ((n = read(client_fd, buff, BUF_SIZE)) < 0)
        {
            perror("read error");
            exit(1);
        }
        buff[n] = '\0';
        printf("%s\n", buff);
        strcpy(send_buff, "150 Opening data connection for directory list");
        write(client_fd, send_buff, strlen(send_buff));
        printf("%s\n", send_buff);

        NLST(buff, send_buff);
        write(data_fd, send_buff, strlen(send_buff));

        strcpy(send_buff, "226 Result is sent successfully");
        write(client_fd, send_buff, strlen(send_buff));
        printf("%s\n", send_buff);
        close(data_fd);
	}

}

int convert_str_to_addr(char *str, struct sockaddr_in *addr)
{
    unsigned long ip = 0;
    unsigned int port = 0;
    char *tmp;
    char *ptr = strtok(str, " ");

    if (!ptr || strcmp(ptr, "PORT"))
        return -1;
    
    if ((ptr = strtok(NULL, " ")) == NULL)
        return -1;

    tmp = ptr;
    ptr = strtok(tmp, ",");
    ip += atoi(ptr) << 24;
    ptr = strtok(NULL, ",");
    ip += atoi(ptr) << 16;
    ptr = strtok(NULL, ",");
    ip += atoi(ptr) << 8;
    ptr = strtok(NULL, ",");
    ip += atoi(ptr);
    ptr = strtok(NULL, ",");
    port += atoi(ptr) << 8;
    ptr = strtok(NULL, ",");
    port += atoi(ptr);

    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_addr.s_addr = htonl(ip);
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;
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
			write(1, print_buf, strlen(print_buf));
			return ;
		}
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
	/////////// if pathname is not a directory or has other problem, dp = NULL //////////
	if ((dp = opendir(pathname)) == NULL)
	{
		///////// if error caused, print error string and exit ////////
		if (errno == EACCES)
			errorM = "cannot access";
		else
			errorM = strerror(errno);
		sprintf(print_buf, "Error : %s\n", errorM);
		write(1, print_buf, strlen(print_buf));
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
				write(1, print_buf, strlen(print_buf));
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
				write(1, print_buf, strlen(print_buf));
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