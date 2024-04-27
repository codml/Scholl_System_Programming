#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFF 4096
#define RCV_BUFF 1024

int     conv_cmd(char *buff, char *cmd_buff);
void    process_result(char *buf);

void main(int argc, char **argv)
{
    int     sockfd, len;
    struct	sockaddr_in server_addr;

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
		write(1, "> ", 2);
		if (read(0, buff, sizeof(buff)) < 0)
		{
			write(2, "read() error\n", strlen("read() error\n"));
			exit(1);
		}
		n = strlen(buff);
		buff[n - 1] = '\0';
        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(2, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            exit(1);
        }
        if (write(sockfd, cmd_buff, n) != n)
        {
            write(2, "write() error!!\n", strlen("write() error!!\n"));
            exit(1);
        }
        if ((n = read(sockfd, rcv_buff, RCV_BUFF - 1) < 0))
        {
            write(2, "read() error\n", strlen("read() error\n"));
            exit(1);
        }
        rcv_buff[n] = '\0';
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
    if (!strcmp(buff, "ls") || !strncmp(buff, "ls ", 3))
	{
		strcpy(cmd_buff, buff);
		memmove(cmd_buff + 2, cmd_buff, strlen(cmd_buff));
		strncpy(cmd_buff, "NLST", strlen("NLST"));
	}
    else if (!strcmp(buff, "quit"))
		strcpy(cmd_buff, "QUIT");
	else
		return -1;
	
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