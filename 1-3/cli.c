////////////////////////////////////////////////////////////////////////
// File Name    :cli.c                                                //
// Date         :2024/04/17                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #1-3: cli              //
// Description  :cli program -> convert usr command to ftp command    //
////////////////////////////////////////////////////////////////////////


#include <unistd.h>
#include <string.h>

#define MAX_BUF 4096

int main(int argc, char **argv)
{
	char buf[MAX_BUF];
	
	// instruction convert table, except rename //
	char *table[][2] = { {"ls", "NLST"},
				{"dir", "LIST"},
				{"pwd", "PWD"},
				{"cd", "CWD"},
				{"mkdir", "MKD"},
				{"delete", "DELE"},
				{"rmdir", "RMD"},
				{"quit", "QUIT"} };

	////// if no arguments, print error ///////
	if (argc == 1)
	{
		strcpy(buf, "Error: cli needs instruction\n");
		write(2, buf, strlen(buf));
		return 1;
	}
		

	///////// buf initialization //////////
	memset(buf, 0, sizeof(buf));

	///////// change usr cmd to ftp cmd(except rename) //////
	for (int i = 0; i < 8; i++)
	{
		if (!strcmp(argv[1], table[i][0]))
		{
			strcat(buf, table[i][1]);
			break;
		}
	}

	/////// for rename ///////
	if (!strcmp(argv[1], "rename"))
	{
		strcat(buf, "RNFR");
		/////// give all option && argument to RNFR except last argument ///////
		for (int i = 2; i < argc - 1; i++)
		{
			strcat(buf, " ");
			strcat(buf, argv[i]);
		}

		strcat(buf, " RNTO ");
		/////// give last argument to RNTO ///////
		strcat(buf, argv[argc - 1]);

		/////// write buf to stdout and stop program //////
		write(1, buf, strlen(buf));
		return 0;
	}

	///////Unknown Instruction///////
	if (!buf[0])
		strcat(buf, "UNKNOWN");

	///////option && argument///////
	for (int i = 2; i < argc; i++)
	{
		////// cd .. -> not CWD, CDUP //////
		if (!strcmp(argv[1], "cd") && !strcmp(argv[i], ".."))
		{
			memmove(buf + 1, buf, strlen(buf));
			strncpy(buf, "CDUP", strlen("CDUP"));
			continue;
		}
		strcat(buf, " ");
		strcat(buf, argv[i]);
	}

	/////// write buf to stdout and stop program //////
	write (1, buf, strlen(buf));
	return 0;
}
