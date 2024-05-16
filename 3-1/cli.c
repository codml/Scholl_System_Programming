#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUF 20
#define CONT_PORT 20001

void log_in(int sockfd);

int main(int argc, char *argv[])
{
    int sockfd, n, p_pid;
    struct sockaddr_in servaddr;

    ///// check the number of arguments /////
	if (argc != 3)
    {
        write(2, "Two arguments are needed: IP, port\n", strlen("Two arguments are needed: IP, port\n"));
        exit(1);
    }

    ////// make socket: domain: IPv4, type: stream(TCP) //////
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	////// set server address structure(including information) //////
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(atoi(argv[2]));

    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    log_in(sockfd);
    close(sockfd);
    return 0;
}

void log_in(int sockfd)
{
    int n;
    char user[MAX_BUF], *passwd, buf[MAX_BUF];

    if ((n = read(sockfd, buf, MAX_BUF)) <= 0)
        return ;
    buf[n] = '\0';
    if (!strcmp(buf, "REJECTION"))
    {
		printf("** Connection refused ""\n");
		close(sockfd);
		exit(1);
	}
	else
		printf("** It is connected to Server **\n");

    while (1)
    {
		write(STDOUT_FILENO, "Input ID : ", strlen("Input ID : "));
		if ((n = read(STDIN_FILENO, buf, MAX_BUF)) <= 0)
			exit(1);
		buf[n - 1] = '\0';
		write(sockfd, buf, strlen(buf));

		strcpy(user, buf);

		write(STDOUT_FILENO, "Input Password : ", strlen("Input Password : "));
        if ((n = read(STDIN_FILENO, buf, MAX_BUF)) <= 0)
			exit(1);
		buf[n - 1] = '\0';
		write(sockfd, buf, strlen(buf));

		n = read(sockfd, buf, MAX_BUF);
		buf[n] = '\0';
        if (!strcmp(buf, "OK"))
        {
            n = read(sockfd, buf, MAX_BUF);
            buf[n] = '\0';

            if (!strcmp(buf, "OK"))
			{
				printf("** User '[%s]' logged in **\n", user);
				break;
			}	
            else if (!strcmp(buf, "FAIL"))
				printf("** Log-in failed **\n");
            else
			{
				printf("** Connection closed **\n");
				break;
			}
				
        }
    }
}