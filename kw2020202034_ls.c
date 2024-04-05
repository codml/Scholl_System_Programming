////////////////////////////////////////////////////////////////////////
// File Name	:kw2020202034_ls.c                                    //
// Date		:2024/4/5                                             //
// OS		:Ubuntu 20.04.6 LTS 64bits                            //
// Author	:Kim Tae Wan                                          //
// Student ID	:2020202034                                           //
// ------------------------------------------------------------------ //
// Title	:System Programming Assignment #1-2                   //
// Description	:...                                                  //
////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	DIR		*dp;
	struct dirent	*dirp;

	char pathname[128];

	if (argc == 1)
		strcpy(pathname, ".");
	else if (argc > 2)
	{
		write(2, "Too many parameter...\n", 22);
		return 0;
	}
	else
		strcpy(pathname, argv[1]);
	if ((dp = opendir(pathname)) == NULL)
	{
		write(2, "can't open ", 11);
		write(2, pathname, strlen(pathname));
		write(2, "\n", 1);
		return 0;
	}
	while (dirp = readdir(dp))
	{
		if (dirp->d_ino != 0)
		{
			write(1, (dirp->d_name), strlen(dirp->d_name));
			write(1, "\n", 1);
		}
	}
	closedir(dp);
	return 0;
}
