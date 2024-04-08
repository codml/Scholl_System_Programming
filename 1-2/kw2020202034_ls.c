////////////////////////////////////////////////////////////////////////
// File Name    :kw2020202034_ls.c                                    //
// Date         :2024/04/08                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #1-2: ls               //
// Description  :...                                                  //
////////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	DIR		*dp;
	struct dirent	*dirp;
	char		*errorM;

	char pathname[128];

	////////////argument number check//////////////
	if (argc == 1) // read current directory
		strcpy(pathname, ".");
	else if (argc > 2) // get more than 1 path
	{
		fprintf(stderr, "only one directory path can be processed\n"); // print error
		return 1;
	}
	else
		strcpy(pathname, argv[1]); // read argument directory

	////////////read file names in the directory////////////
	if ((dp = opendir(pathname)) == NULL) // fail to open directory stream
	{
		if (errno == EACCES) // Permission denied
			errorM = "Access denied";
		else if (errno == ENOENT) // no such directory or name is empty string
			errorM = "No such directory";
		else
			errorM = strerror(errno);
		fprintf(stderr, "testls: cannot access '%s': %s\n", pathname, errorM);
		return 1;
	}
	while (dirp = readdir(dp)) // read until meet NULL -> means read all directory entries
	{
		if (dirp->d_ino != 0)
			printf("%s\n", dirp->d_name); // print file name
	}
	closedir(dp); // close directory stream
	return 0;
}
