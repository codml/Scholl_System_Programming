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

void	MtoS(mode_t st_mode, char *string);
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

void	MtoS(mode_t st_mode, char *str)
{
	if (S_ISREG(st_mode))
		str[0] = '-';
	else if (S_ISDIR(st_mode))
		str[0] = 'd';
	else if (S_ISCHR(st_mode))
		str[0] = 'c';
	else if (S_ISBLK(st_mode))
		str[0] = 'b';
	else if (S_ISFIFO(st_mode))
		str[0] = 'p';
	else if (S_ISLNK(st_mode))
		str[0] = 'l';
	else if (S_ISSOCK(st_mode))
		str[0] = 's';
	str[1] = (st_mode & S_IRUSR) ? 'r' : '-';
    str[2] = (st_mode & S_IWUSR) ? 'w' : '-';
    str[3] = (st_mode & S_IXUSR) ? 'x' : '-';
    str[4] = (st_mode & S_IRGRP) ? 'r' : '-';
    str[5] = (st_mode & S_IWGRP) ? 'w' : '-';
    str[6] = (st_mode & S_IXGRP) ? 'x' : '-';
    str[7] = (st_mode & S_IROTH) ? 'r' : '-';
    str[8] = (st_mode & S_IWOTH) ? 'w' : '-';
    str[9] = (st_mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

void	NLST(char *buf)
{
	char 			c;
	int				idx;
	int				aflag = 0;
	int				lflag = 0;

	char			*pathname;
	char			*errorM;

	DIR				*dp;
	struct dirent	*dirp;
	struct stat		infor;


	char			*split[256];
	char			mode[11];
	char			time_buf[32];
	char			print_buf[MAX_BUF];
	opterr = 0;

	strcpy(print_buf, buf);
	for (char *ptr = strtok(print_buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	while ((c = getopt(idx, split, "al")) != -1)
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
			write(2, "Error: invalid option\n", 100);
			exit(1);
		}
	}
	if (optind != idx && optind != idx - 1)
	{
		write(2, "Error: too many arguments\n", 100);
		exit(1);
	}
	if (optind == idx)
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
			MtoS(infor.st_mode, mode);
			strftime(time_buf, sizeof(time_buf), "%b %d %R",
				localtime(&(infor.st_mtime)));
			snprintf(print_buf, sizeof(print_buf),
				"%s %2ld %s %s %6ld %s %s\n",
				mode, infor.st_nlink, getpwuid(infor.st_uid)->pw_name,
				getgrgid(infor.st_gid)->gr_name, infor.st_size,
				time_buf, pathname);
			write(1, buf, strlen(buf));
			write(1, "\n", 1);
			write(1, print_buf, strlen(print_buf));
			exit(0);
		}
		snprintf(print_buf, sizeof(print_buf), "Error : %s\n", strerror(errno));
		write(2, print_buf, strlen(print_buf));
		exit(1);
	}
	write(1, "Not Yet\n", 100);
	exit(0);
}

void	LIST(char *buf)
{
	exit(0);
}

void	PWD(char *buf)
{
	exit(0);
}

void	CWD(char *buf)
{
	exit(0);
}

void	CDUP(char *buf)
{
	exit(0);
}

void	MKD(char *buf)
{
	exit(0);
}

void	DELE(char *buf)
{
	exit(0);
}

void	RMD(char *buf)
{
	exit(0);
}

void	RN(char *buf)
{
	exit(0);
}

void	QUIT(char *buf)
{
	exit(0);
}

void	UNK(char *buf)
{
	char	*error = "Unknown command\n";
	write(2, error, strlen(error));
	exit(1);
}
