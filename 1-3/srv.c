#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 4096

void	NLST(const char *buf);
void	LIST(const char *buf);
void	PWD(const char *buf);
void	CWD(const char *buf);
void	CDUP(const char *buf);
void	MKD(const char *buf);
void	DELE(const char *buf);
void	RMD(const char *buf);
void	RN(const char *buf);
void	QUIT(const char *buf);
void	UNK(const char *buf);

struct s_table {
	char	*func_name;
	void	(*fp)(const char *);
};

void main()
{
	char	buf[MAX_BUF];
	int		idx = 0;

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

void	NLST(const char *buf)
{
	char	*split[256];
	int		idx = 0;

	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;

	
	exit(0);
}

void	LIST(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	PWD(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	CWD(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	CDUP(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	MKD(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	DELE(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	RMD(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	RN(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	QUIT(const char *buf)
{
	for (char *ptr = strtok(buf, " "); ptr; ptr = strtok(NULL, " "))
		split[idx++] = ptr;
	split[idx] = NULL;
	exit(0);
}

void	UNK(const char *buf)
{
	char	*error = "Unknown command\n";
	write(2, error, strlen(error));
	exit(1);
}
