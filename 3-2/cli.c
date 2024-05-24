#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define BUF_SIZE 4096

void convert_addr_to_str(char *buf, struct sockaddr_in *tmp);

void main(int argc, char **argv)
{
    char *hostport;
	char buff[BUF_SIZE], portcmd[BUF_SIZE], cmd[BUF_SIZE];
    int ctrlfd, datafd, dataconfd;
	int n, port;
    struct sockaddr_in temp;

    ///// check the number of arguments /////
	if (argc != 3)
    {
        write(2, "Two arguments are needed: IP, port\n", strlen("Two arguments are needed: IP, port\n"));
        exit(1);
    }

    ctrlfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&temp, 0, sizeof(temp));
	temp.sin_family=AF_INET;
	temp.sin_addr.s_addr=inet_addr(argv[1]);
	temp.sin_port=htons(atoi(argv[2]));

    ///// connect to server using serv_addr. if failed, write error and exit /////
	if (connect(ctrlfd,(struct sockaddr *)&temp,sizeof(temp)) < 0)
	{
		perror("connect error");
		exit(1);
	}
    while (1)
    {
		memset(buff, 0, BUF_SIZE);

        ///// write string which client will send to server /////
        write(STDOUT_FILENO, "> ", 2);
        if ((n = read(STDIN_FILENO, buff, BUF_SIZE)) < 0)
        {
            perror("Read error");
            exit(1);
        }
		buff[n - 1] = '\0';

		if (strcmp(buff, "ls")) // need to fix
			strcpy(cmd, "NLST");
		else
			strcpy(cmd, "UNKNOWN");

		port = 10001 + time(NULL) % 20000;
		memset(&temp, 0, sizeof(temp));
		temp.sin_family=AF_INET;
		temp.sin_addr.s_addr=htonl(INADDR_ANY);
		temp.sin_port=htons(port);

		datafd = socket(AF_INET, SOCK_STREAM, 0);
		bind(datafd, (struct sockaddr *)&temp, sizeof(temp));
		listen(datafd, 5);
		
        convert_addr_to_str(portcmd, &temp);
		if (write(ctrlfd, portcmd, strlen(portcmd)) <= 0)
		{
			perror("write error");
			exit(1);
		}
		n = sizeof(temp);
		if ((dataconfd = accept(datafd, (struct sockaddr *)&temp, &n)) < 0)
		{
			perror("data connection error");
			exit(1);
		}
		close(datafd);

		if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
		{
			perror("read error");
			exit(1);
		}
		buff[n] = '\0';
		printf("%s\n", buff);

		if (write(ctrlfd, cmd, strlen(cmd)) < 0)
		{
			perror("write error");
			exit(1);
		}

		if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
		{
			perror("read error");
			exit(1);
		}
		buff[n] = '\0';
		printf("%s\n", buff);

		if ((n = read(ctrlfd, buff, BUF_SIZE)) < 0)
		{
			perror("read error");
			exit(1);
		}
		buff[n] = '\0';
		write(STDOUT_FILENO, buff, strlen(buff));
		close(dataconfd);
    }
}

void convert_addr_to_str(char *buf, struct sockaddr_in *tmp)
{
	char *bu, *ptr;

	strcpy(buf, "PORT ");
	strcat(buf, inet_ntoa(tmp->sin_addr));
	bu = buf;
	while (ptr = strchr(buf, '.'))
		*ptr = ',';
	sprintf(buf + strlen(buf), ",%d,%d", (tmp->sin_port) >> 8, (tmp->sin_port) & 256);
}