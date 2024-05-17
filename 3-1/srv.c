#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pwd.h>

#define MAX_BUF 20

int log_auth(int connfd);
int user_match(char *user, char *passwd);

int main(int argc, char *argv[])
{
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    FILE *fp_checkIP;

    ///// check the number of arguments /////
    if (argc != 2)
    {
        write(2, "One argument is needed: port\n", strlen("one argument is needed: port\n"));
        exit(1);
    }

    ///// make socket for server //////
    listenfd = socket(PF_INET, SOCK_STREAM, 0);

    ////// set server address for bind //////
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(atoi(argv[1]));

    ///// bind server address with server socket /////
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    listen(listenfd, 5);
    while (1)
    {
        int clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		printf("** Client is connected **\n");
		printf(" - IP: %s\n - Port: %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

		if((fp_checkIP = fopen("access.txt", "r")) == NULL)
			exit(1);
		
		char buf[MAX_BUF];
		char cli_ip[MAX_BUF];
		strcpy(cli_ip, inet_ntoa(cliaddr.sin_addr));
		char *cli_ips[5];
		cli_ips[0] = strtok(cli_ip, ".");
		for (int i = 1; (cli_ips[i] = strtok(NULL, ".")) != NULL; i++);
		
		char *ptr, *tmp;
		while ((ptr = fgets(buf, MAX_BUF, fp_checkIP)) != NULL)
		{
			tmp = strtok(buf, ".");
			if (strcmp(tmp, cli_ips[0]) && strcmp(tmp, "*"))
				continue;
			tmp = strtok(NULL, ".");
			if (strcmp(tmp, cli_ips[1]) && strcmp(tmp, "*"))
				continue;
			tmp = strtok(NULL, ".");
			if (strcmp(tmp, cli_ips[2]) && strcmp(tmp, "*"))
				continue;
			tmp = strtok(NULL, ".");
			if (strcmp(tmp, cli_ips[3]) && strcmp(tmp, "*"))
				continue;
			break;
		}
		fclose(fp_checkIP);
		if (!ptr)
		{
			printf("** It is NOT authenticated client **\n");
			write(connfd, "REJECTION", MAX_BUF);
			close(connfd);
			continue;
		}
		else
			write(connfd, "ACCEPTED", MAX_BUF);

        if (log_auth(connfd) == 0)
        {
            printf("** Fail to log-in **\n");
            close(connfd);
            continue;
        }
        printf("** Success to log-in **\n");
        close(connfd);
    }
}

int log_auth(int connfd)
{
    char user[MAX_BUF], passwd[MAX_BUF];
    int n, count = 1;

    while(1)
    {
		if ((n = read(connfd, user, MAX_BUF)) <= 0)
			return 0;
		user[n] = '\0';
		if ((n = read(connfd, passwd, MAX_BUF)) <= 0)
			return 0;
		passwd[n] = '\0';
        write(connfd, "OK", MAX_BUF);

		printf("** User is trying to log-in (%d/3) **\n", count);

        if ((n = user_match(user, passwd)) == 1)
		{
			write(connfd, "OK", MAX_BUF);
			return 1;
		}
        else if (n == 0)
        {
			printf("** Log-in failed **\n");
            if (count >= 3)
            {
				write(connfd, "DISCONNECTION", MAX_BUF);
				return 0;
            }
            write(connfd, "FAIL", MAX_BUF);
            count++;
            continue;
        }
    }
    return 1;
}

int user_match(char *user, char *passwd)
{
    FILE *fp;
    struct passwd *pw;

    fp = fopen("passwd", "r");

	while ((pw = fgetpwent(fp)) != NULL)
	{
		if (!strcmp(user, pw->pw_name) && !strcmp(passwd, pw->pw_passwd))
			return 1;
	}
	fclose(fp);
	return 0;
}