#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#define MAX_BUFF 4096
#define RCV_BUFF 4096

int     conv_cmd(char *buff, char *cmd_buff);
void    process_result(char *buf);

void main(int argc, char **argv)
{
    int     sockfd, len;
    struct	sockaddr_in server_addr;
	char	*str;

    char    buff[MAX_BUFF], cmd_buff[MAX_BUFF], rcv_buff[RCV_BUFF];
    int     n;

	if (argc != 3)
	{
		write(2, "Format: ./cli [server addr] [port num]\n", strlen("Format: ./cli [server addr] [port num]\n"));
		exit(1);
	}

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		write(2, "can't create socket\n", strlen("can't create socket\n"));
		exit(1);
	}

	memset((char *)&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		write(2, "can't connect\n", strlen("can't connect\n"));
		exit(1);
	}

    while (1)
    {
		memset(buff, 0, sizeof(buff));
		memset(cmd_buff, 0, sizeof(cmd_buff));
		memset(rcv_buff, 0, sizeof(rcv_buff));
		write(1, "> ", 2);
		if ((n = read(0, buff, sizeof(buff))) < 0)
		{
			write(2, "read() error\n", strlen("read() error\n"));
			exit(1);
		}
		buff[n] = '\0';
        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(2, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            exit(1);
        }
		n = strlen(cmd_buff);
        if (write(sockfd, cmd_buff, n) != n)
        {
            write(2, "write() error!!\n", strlen("write() error!!\n"));
            exit(1);
        }
        while ((n = read(sockfd, rcv_buff, RCV_BUFF - 1) > 0))
        {
            if (str = strchr(rcv_buff, '\0'))
				break;
        }
		if (n <= 0)
		{
			write(2, "read() error\n", strlen("read() error\n"));
			exit(1);
		}
        if (!strcmp(rcv_buff, "QUIT"))
        {
            write(1, "Program quit!!\n", strlen("Program quit!!\n"));
			close(sockfd);
			exit(0);
        }

        process_result(rcv_buff);
    }
}

int conv_cmd(char *buff, char *cmd_buff)
{
	int		len = 0;
	char	*split[256];

    for (char *ptr = strtok(buff, " \n"); ptr; ptr = strtok(NULL, " \n"))
		split[len++] = ptr;
	split[len] = NULL;
	if (!strcmp(split[0], "ls"))
		strcpy(cmd_buff, "NLST");
	else if (!strcmp(split[0], "quit") && len == 1)
		strcpy(cmd_buff, "QUIT");
	else
		return -1;
	for (int i = 1; i < len; i++)
	{
		strcat(cmd_buff, " ");
		strcat(cmd_buff, split[i]);
	}
	return 0;
}

void process_result(char *buf)
{
	if (write(1, buf, strlen(buf)) < 0)
	{
		write(2, "write() error\n", strlen("write() error\n"));
		exit(1);
	}
}