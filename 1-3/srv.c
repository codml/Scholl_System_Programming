#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MAX_BUF 4096

void	MtoS(mode_t st_mode, char *string);
void	NLST(char **buf, int argc);
void	LIST(char **buf, int argc);
void	PWD(char **buf, int argc);
void	CWD(char **buf, int argc);
void	CDUP(char **buf, int argc);
void	MKD(char **buf, int argc);
void	DELE(char **buf, int argc);
void	RMD(char **buf, int argc);
void	RN(char **buf, int argc);
void	QUIT(char **buf, int argc);
void	UNK(char **buf, int argc);

struct	s_table {
	char	*func_name;
	void	(*fp)(char**, int);
};

void	main()
{
	char	buf[MAX_BUF];
	int		idx = 0;
	char	*split[256];
	int		idx = 0;
	char	c;

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

	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;

	for (int i = 0; i < 11; i++)
	{
		if (!strncmp(split[0], table[i].func_name, strlen(table[i].func_name)))
			(table[i].fp)(split, idx); // each function has exit()
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

void	NLST(char ** buf, int argc)
{
	char 			c;
	int				aflag = 0;
	int				lflag = 0;
	char			*pathname;
	DIR				*dp;
	struct dirent	*dirp;
	struct stat		infor;
	char			mode[11];
	opterr = 0;

	while ((c = getopt(argc, buf, "al")) != -1)
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
	if (optind != argc && optind != argc - 1)
	{
		write(2, "Error: too many arguments\n", 100);
		exit(1);
	}
	if (optind == argc)
		pathname = ".";
	else
		pathname = buf[optind];
	
	if ((dp = opendir(pathname)) == NULL)
	{
		if (errno == ENOTDIR && lflag)
		{
			if(stat(pathname, &infor) == -1)
			{
				// error handling for reading file
			}
			MtoS(infor.st_mode, &mode);
			exit(0);
		}
		write(2, strerror(errno), strlen(strerror(errno)));
		exit(1);
	}

	exit(0);
}

void	LIST(char ** buf, int argc)
{

	exit(0);
}

void	PWD(char ** buf, int argc)
{
	exit(0);
}

void	CWD(char ** buf, int argc)
{

	exit(0);
}

void	CDUP(char ** buf, int argc)
{

	exit(0);
}

void	MKD(char ** buf, int argc)
{

	exit(0);
}

void	DELE(char ** buf, int argc)
{

	exit(0);
}

void	RMD(char ** buf, int argc)
{

	exit(0);
}

void	RN(char ** buf, int argc)
{

	exit(0);
}

void	QUIT(char ** buf, int argc)
{

	exit(0);
}

void	UNK(char ** buf, int argc)
{
	char	*error = "Unknown command\n";
	write(2, error, strlen(error));
	exit(1);
}
