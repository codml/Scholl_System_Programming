#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	DIR		*dp;
	struct dirent	*dirp;

	char pathname[128];

	if (argc == 1)
		strcpy(pathname, ".");
	else if (argc > 2)
	{
		fprintf(stderr, "Too many parameters...\n");
		return 0;
	}
	else
		strcpy(pathname, argv[1]);
	if ((dp = opendir(pathname)) == NULL)
	{
		fprintf(stderr, "testls: cannot access '%s': %s\n", pathname, strerror(errno));
		return 0;
	}
	while (dirp = readdir(dp))
	{
		if (dirp->d_ino != 0)
			printf("%s\n", dirp->d_name);
	}
	closedir(dp);
	return 0;
}
