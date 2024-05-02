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

#define BUF_SIZE 256

int main(int argc, char **argv)
{
    char buff[BUF_SIZE];
	int n;
	int sockfd;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		write(2, "connect error\n", strlen("connect error\n"));
		exit(1);
	}
		
	while (1)
	{
		memset(buff, 0, BUF_SIZE);
		write(STDOUT_FILENO, "> ", 2);
		read(STDIN_FILENO, buff, BUF_SIZE);

		if (write(sockfd, buff, BUF_SIZE) > 0)
		{
			if (read(sockfd, buff, BUF_SIZE) > 0)
				printf("from server:%s", buff);
			else
				break;
		}
		else
			break;
	}
	close(sockfd);
	return 0;
}
