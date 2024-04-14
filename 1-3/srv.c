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

void	MtoS(struct stat *infor, char *pathname, char *print_buf);
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
void	UNK(char *buf);

struct	s_table {
	char	*func_name;
	void	(*fp)(char*);
};

void	main()
{
	char	buf[MAX_BUF];

	struct s_table table[11] = {
		{"NLST", NLST},
		{"LIST", LIST},
		{"PWD", PWD},
		{"CWD", CWD},
		{"CDUP", CDUP},
		{"MKD", MKD},
		{"DELE", DELE},
		{"RMD", RMD},
		{"RNFR", RN},
		{"QUIT", QUIT},
		{"UNK", UNK}
	};

	memset(buf, 0, sizeof(buf));
	read(0, buf, sizeof(buf));

	for (int i = 0; i < 11; i++)
	{
		if (!strncmp(buf, table[i].func_name, strlen(table[i].func_name)))
			(table[i].fp)(buf); // each function has exit()
	}
}

void	MtoS(struct stat *infor, char *pathname, char *print_buf)
{
	char	time_buf[32];
	char	str[11];

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
	str[1] = (infor->st_mode & S_IRUSR) ? 'r' : '-';
    str[2] = (infor->st_mode & S_IWUSR) ? 'w' : '-';
    str[3] = (infor->st_mode & S_IXUSR) ? 'x' : '-';
    str[4] = (infor->st_mode & S_IRGRP) ? 'r' : '-';
    str[5] = (infor->st_mode & S_IWGRP) ? 'w' : '-';
    str[6] = (infor->st_mode & S_IXGRP) ? 'x' : '-';
    str[7] = (infor->st_mode & S_IROTH) ? 'r' : '-';
    str[8] = (infor->st_mode & S_IWOTH) ? 'w' : '-';
    str[9] = (infor->st_mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
	strftime(time_buf, sizeof(time_buf), "%b %d %R",
				localtime(&(infor->st_mtime)));
	if (S_ISDIR(infor->st_mode))
		snprintf(print_buf, MAX_BUF,
		"%s %2ld %s %s %5ld %s %s/\n",
		str, infor->st_nlink, getpwuid(infor->st_uid)->pw_name,
		getgrgid(infor->st_gid)->gr_name, infor->st_size,
		time_buf, pathname);
	else
		snprintf(print_buf, MAX_BUF,
			"%s %2ld %s %s %5ld %s %s\n",
			str, infor->st_nlink, getpwuid(infor->st_uid)->pw_name,
			getgrgid(infor->st_gid)->gr_name, infor->st_size,
			time_buf, pathname);
}

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


	char			*split[256];
	char			*filename[256];

	char			print_buf[MAX_BUF];
	opterr = 0;

	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

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

	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	if (optind == len)
		pathname = ".";
	else
		pathname = split[optind];
	
	if ((dp = opendir(pathname)) == NULL)
	{
		if (errno == ENOTDIR && lflag)
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
			strcat(buf, "\n");
			write(1, buf, strlen(buf));
			write(1, print_buf, strlen(print_buf));
			exit(0);
		}
		snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
		write(2, print_buf, strlen(print_buf));
		exit(1);
	}
	idx = 0;
	while (dirp = readdir(dp)) // read until meet NULL -> means read all directory entries
	{
		if (dirp->d_ino != 0)
			filename[idx++] = dirp->d_name;
	}
	
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

	idx = len;
	for (int i = optind; i < len; i++)
	{
		if (filename[i][0] != '.')
		{
			idx = i;
			break;
		}
	}

	if (aflag)
		start_idx = 0;
	else
		start_idx = idx;
	
	strcat(buf, "\n");
	write(1, buf, strlen(buf));
	if (lflag)
	{
		for (int i = start_idx; i < len; i++)
		{
			stat(filename[i], &infor);
			MtoS(&infor, filename[i], print_buf);
			write(1, print_buf, strlen(print_buf));
		}
	}
	else
	{
		for (int i = start_idx; i < len; i++)
		{
			write(1, filename[i], strlen(filename[i]));
			if ((i - start_idx) % 5 == 4)
				write(1, "\n", 1);
			else
				write(1, " ", 1);
		}
		write(1, "\n", 1);
	}
	exit(0);
}

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

	char			print_buf[MAX_BUF];

	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	if (optind != len && optind != len - 1)
	{
		errorM = "Error: too many arguments\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	if (optind == len)
		pathname = ".";
	else
		pathname = split[optind];
	
	if ((dp = opendir(pathname)) == NULL)
	{
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
			strcat(buf, "\n");
			write(1, buf, strlen(buf));
			write(1, print_buf, strlen(print_buf));
			exit(0);
		}
		snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
		write(2, print_buf, strlen(print_buf));
		exit(1);
	}

	idx = 0;
	while (dirp = readdir(dp)) // read until meet NULL -> means read all directory entries
	{
		if (dirp->d_ino != 0)
			filename[idx++] = dirp->d_name;
	}
	
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
	
	strcat(buf, "\n");
	write(1, buf, strlen(buf));
	for (int i = 0; i < len; i++)
	{
		stat(filename[i], &infor);
		MtoS(&infor, filename[i], print_buf);
		write(1, print_buf, strlen(print_buf));
	}

	exit(0);
}

void	PWD(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	char	print_buf[MAX_BUF];
	opterr = 0;

	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	if (len > 1)
	{
		errorM = "Error: argument is not required\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	getcwd(path_buf, sizeof(path_buf));

	strcat(buf, "\n");
	write(1, buf, strlen(buf));
	snprintf(print_buf, MAX_BUF, "\"%s\" is current directory\n", path_buf);
	write(1, print_buf, strlen(print_buf));

	exit(0);
}

void	CWD(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	char	print_buf[MAX_BUF];
	opterr = 0;

	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	if (len == 1)
	{
		errorM = "Error: one argument is equired\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	if (len > 2)
	{
		errorM = "Error: too much argument\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

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
	
	getcwd(path_buf, sizeof(path_buf));
	
	strcat(buf, "\n");
	write(1, buf, strlen(buf));
	snprintf(print_buf, MAX_BUF, "\"%s\" is current directory\n", path_buf);
	write(1, print_buf, strlen(print_buf));

	exit(0);
}

void	CDUP(char *buf)
{
	int		len = 0;
	char	*errorM;

	char	*split[256];
	char	path_buf[257];
	char	print_buf[MAX_BUF];
	opterr = 0;

	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

	if (len > 1)
	{
		errorM = "Error: too much argument\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}

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
	
	getcwd(path_buf, sizeof(path_buf));
	
	strcat(buf, "\n");
	write(1, buf, strlen(buf));
	snprintf(print_buf, MAX_BUF, "\"%s\" is current directory\n", path_buf);
	write(1, print_buf, strlen(print_buf));
	exit(0);
}

void	MKD(char *buf)
{
	int				len = 0;
	char			*errorM;

	char			*split[256];
	char			print_buf[MAX_BUF];
	opterr = 0;
	
	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}
	
	exit(0);
}

void	DELE(char *buf)
{
	int				len = 0;
	char			*errorM;

	char			*split[256];
	char			print_buf[MAX_BUF];
	opterr = 0;
	
	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}
	exit(0);
}

void	RMD(char *buf)
{
	int				len = 0;
	char			*errorM;

	char			*split[256];
	char			print_buf[MAX_BUF];
	opterr = 0;
	
	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}
	exit(0);
}

void	RN(char *buf)
{
	int				len = 0;
	char			*errorM;

	char			*split[256];
	char			print_buf[MAX_BUF];
	opterr = 0;
	
	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}
	exit(0);
}

void	QUIT(char *buf)
{
	int				len = 0;
	char			*errorM;

	char			*split[256];
	char			print_buf[MAX_BUF];
	opterr = 0;
	
	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[len++] = ptr;
	split[len] = NULL;

	while (getopt(len, split, "") != -1)
	{
		errorM = "Error: invalid option\n";
		write(2, errorM, strlen(errorM));
		exit(1);
	}
	exit(0);
}

void	UNK(char *buf)
{
	char	*error = "Unknown command\n";
	write(2, error, strlen(error));
	exit(1);
}
