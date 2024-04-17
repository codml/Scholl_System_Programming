////////////////////////////////////////////////////////////////////////
// File Name    :srv.c                                                //
// Date         :2024/04/17                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #1-3: srv              //
// Description  :execute ftp commands read from standard input        //
////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define MAX_BUF 4096

void	MtoS(struct stat *infor, const char *pathname, char *print_buf);
void	NLST(char *buf);
void	LIST(char *buf);
void	PWD(char *buf);
void	CWD(char *buf);
void	CDUP(char *buf);
void	MKD(char *buf);
void	DELE(char *buf);
void	RMD(char *buf);
void	RN(char *buf);
void	QUIT(char *buf);


////// a structure for fucntion name table //////
struct	s_table {
	char	*func_name;
	void	(*fp)(char*);
};

void	main()
{
	char	buf[MAX_BUF];

	////// function name : function pointer dictionary //////
	struct s_table table[10] = {
		{"NLST", NLST},
		{"LIST", LIST},
		{"PWD", PWD},
		{"CWD", CWD},
		{"CDUP", CDUP},
		{"MKD", MKD},
		{"DELE", DELE},
		{"RMD", RMD},
		{"RNFR", RN},
		{"QUIT", QUIT}
	};

	memset(buf, 0, sizeof(buf));
	
	////// read FTP command from standard input ///////
	read(0, buf, sizeof(buf));

	for (int i = 0; i < 10; i++)
	{
		/////// if command in buf matches with function name, execute the function //////
		if (!strncmp(buf, table[i].func_name, strlen(table[i].func_name)))
			(table[i].fp)(buf); // each function has exit(), so no break
	}
	
	////// print error for unknown command ///////
	char	*error = "Unknown command\n";
	write(2, error, strlen(error));
	exit(1);
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
		snprintf(print_buf, MAX_BUF, "%s %2ld %s %s %6ld %s %s/\n",
			str, infor->st_nlink, getpwuid(infor->st_uid)->pw_name,
			getgrgid(infor->st_gid)->gr_name, infor->st_size,
			time_buf, pathname);
	}
	else
	{
		snprintf(print_buf, MAX_BUF, "%s %2ld %s %s %6ld %s %s\n",
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

void	NLST(char *buf)
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
	char			tmp_buf[MAX_BUF];
	char			print_buf[MAX_BUF];
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
			write(2, errorM, strlen(errorM));
			exit(1);
		}
	}

	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	/////////// print command(NLST) and  options //////
	for (int i = 0; i < optind; i++)
	{
		write(1, split[i], strlen(split[i]));
		write(1, " ", 1);
	}
	write(1, "\n", 1);

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
		if (errno == ENOTDIR && lflag && !aflag)
		{
			//////// load file status in struct stat infor ////////
			if (stat(pathname, &infor) == -1)
			{
				/////// error handling when failed to load file status ///////
				snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
				write(2, print_buf, strlen(print_buf));
				exit(1);
			}
			////////// print formatted file status string and exit //////////
			MtoS(&infor, pathname, print_buf);
			write(1, print_buf, strlen(print_buf));
			exit(0);
		}
		///////// if error caused by other problem, print error string and exit ////////
		if (errno == EACCES)
			errorM = "cannot access";
		else
			errorM = strerror(errno);
		snprintf(print_buf, sizeof(print_buf), "Error : %s\n", errorM);
		write(2, print_buf, strlen(print_buf));
		exit(1);
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
			stat(filename[i], &infor);
			MtoS(&infor, filename[i], print_buf);
			write(1, print_buf, strlen(print_buf));
		}
	}
	else ////// if no '-l' option, just print file names //////
	{
		for (int i = start_idx; i < len; i++)
		{
			write(1, filename[i], strlen(filename[i]));
			stat(filename[i], &infor);
			if (S_ISDIR(infor.st_mode)) // if the file is directory, write '/' behind its name
				write(1, "/", 1);
			if ((i - start_idx) % 5 == 4)
				write(1, "\n", 1);
			else
				write(1, " ", 1);
		}
		write(1, "\n", 1);
	}
	closedir(dp);
	exit(0);
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

void	LIST(char *buf)
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
	char			print_buf[MAX_BUF];
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are not zero or one -> error ////////////
	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	///////// print command name //////////
	write(1, split[0], strlen(split[0]));
	write(1, "\n", 1);

	/////////// set pathname, if argument is none, '.'(current dir) is pathname ////////////
	if (optind == len)
		pathname = ".";
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
				snprintf(print_buf, sizeof(print_buf), "Error : %s\n", errorM);
				write(2, print_buf, strlen(print_buf));
				exit(1);
			}
			MtoS(&infor, pathname, print_buf);
			write(1, print_buf, strlen(print_buf));
			exit(0);
		}
		///////// if error caused by other problem, print error string and exit ////////
		snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
		write(2, print_buf, strlen(print_buf));
		exit(1);
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
		stat(filename[i], &infor);
		MtoS(&infor, filename[i], print_buf);
		write(1, print_buf, strlen(print_buf));
	}
	closedir(dp);
	exit(0);
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

void	PWD(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	char	tmp_buf[MAX_BUF];
	char	print_buf[MAX_BUF];
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are not zero -> error ////////////
	if (len > 1)
	{
		errorM = "Error: argument is not required\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// get current working directory in path_buf ///////////
	getcwd(path_buf, sizeof(path_buf));

	//////////// print current working directory ////////////
	snprintf(print_buf, MAX_BUF, "\"%s\" is current directory\n", path_buf);
	write(1, print_buf, strlen(print_buf));
	exit(0);
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

void	CWD(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	char	tmp_buf[MAX_BUF];
	char	print_buf[MAX_BUF];
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are more than one -> error ////////////
	if (len > 2)
	{
		errorM = "Error: too much argument\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	///////////// change working directory to argv[1]. if failed, print error and exit //////////////
	if (chdir(split[1])< 0)
	{
		if (errno == ENOENT)
			errorM = "directory not found";
		else
			errorM = strerror(errno);
		snprintf(print_buf, MAX_BUF, "Error: %s\n", errorM);
		write(2, print_buf, strlen(print_buf));
		exit(1);
	}
	
	/////////// to check the result, get current working directory ///////////////
	getcwd(path_buf, sizeof(path_buf));
	
	/////// print command ///////
	strcat(buf, "\n");
	write(1, buf, strlen(buf));

	///////// print current working directory //////////
	snprintf(print_buf, MAX_BUF, "\"%s\" is current directory\n", path_buf);
	write(1, print_buf, strlen(print_buf));
	exit(0);
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

void	CDUP(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	char	print_buf[MAX_BUF];
	opterr = 0;

	///////////// split a command by space && stored in split[] ////////////
	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	///////////// option parsing -> if option exists, error ///////////
	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are not zero -> error ////////////
	if (len > 1)
	{
		errorM = "Error: too much argument\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	////////////// change directory to parent directory. if failed, print error message ///////////////
	if (chdir("..")< 0)
	{
		if (errno == ENOENT)
			errorM = "directory not found";
		else
			errorM = strerror(errno);
		snprintf(print_buf, MAX_BUF, "Error: %s\n", errorM);
		write(2, print_buf, strlen(print_buf));
		exit(1);
	}
	
	/////////// to check the result, get current working directory ///////////////
	getcwd(path_buf, sizeof(path_buf));
	
	///////// print command, current working directory //////////
	strcat(buf, "\n");
	write(1, buf, strlen(buf));
	snprintf(print_buf, MAX_BUF, "\"%s\" is current directory\n", path_buf);
	write(1, print_buf, strlen(print_buf));
	exit(0);
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

void	MKD(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	print_buf[MAX_BUF];
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		write(2, errorM, strlen(errorM));
		exit(1);
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
			snprintf(print_buf, MAX_BUF, "Error: cannot create directory \'%s\': %s\n", split[i], errorM);
			write(2, print_buf, strlen(print_buf));
		}
		else
		{
			/// print command, file name ///
			snprintf(print_buf, MAX_BUF, "%s %s\n", split[0], split[i]);
			write(1, print_buf, strlen(print_buf));
		}
	}

	exit(0);
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

void	DELE(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	print_buf[MAX_BUF];
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	/////////// remove each files that correspond with the arguments ///////////
	for (int i = optind; i < len; i++)
	{
		/////// unlink()-> remove file //////////
		if (unlink(split[i]) == -1)
		{
			////// if failed to remove, print error message //////
			snprintf(print_buf, MAX_BUF, "Error: failed to delete \'%s\'\n", split[i]);
			write(2, print_buf, strlen(print_buf));
		}
		else
		{
			/////// print result ////////
			snprintf(print_buf, MAX_BUF, "%s %s\n", split[0], split[i]);
			write(1, print_buf, strlen(print_buf));
		}
	}
	exit(0);
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

void	RMD(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	print_buf[MAX_BUF];
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are zero -> error ////////////
	if (len == 1)
	{
		errorM = "Error: argument is required\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// remove empty directories that correspond with the arguments //////////
	for (int i = optind; i < len; i++)
	{
		/////// rmdir() -> remove empty direcoty. if failed -> print error ///////
		if (rmdir(split[i]) == -1)
		{
			snprintf(print_buf, MAX_BUF, "Error: failed to remove \'%s\'\n", split[i]);
			write(2, print_buf, strlen(print_buf));
		}
		else
		{
			//////// print result /////////
			snprintf(print_buf, MAX_BUF, "%s %s\n", split[0], split[i]);
			write(1, print_buf, strlen(print_buf));
		}
	}
	exit(0);
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

void	RN(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	print_buf[MAX_BUF];
	struct stat infor;
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are not two(old, new) -> error ////////////
	if (len != 4 | strcmp(split[2], "RNTO"))
	{
		errorM = "Error: two argumens are required\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	////// if file exist, stat() will return 0  -> rename error //////
	if (!stat(split[3], &infor))
	{
		errorM = "Error: name to change already exists\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	////// if failed to rename, print error and exit //////
	if (rename(split[1], split[3]) == -1)
	{
		snprintf(print_buf, MAX_BUF, "Error: %s\n", strerror(errno));
		write(2, print_buf, strlen(print_buf));
		exit(1);
	}

	////// print result //////
	snprintf(print_buf, MAX_BUF, "%s %s\n%s %s\n", split[0], split[1], split[2], split[3]);
	write(1, print_buf, strlen(print_buf));
	exit(0);
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

void	QUIT(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	tmp_buf[MAX_BUF];
	char	print_buf[MAX_BUF];
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
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	//////////// if # of arguments are not zero -> error ////////////
	if (len != 1)
	{
		errorM = "Error: argument is not required\n";
		write(1, errorM, strlen(errorM));
		exit(1);
	}

	////// print result ///////
	snprintf(print_buf, MAX_BUF, "%s success\n", split[0]);
	write(1, print_buf, strlen(print_buf));
	exit(0);
}
