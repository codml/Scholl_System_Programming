////////////////////////////////////////////////////////////////////////
// File Name    :srv.c                                                //
// Date         :2024/05/20                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #3-1: server with login//
// Description  :server - accept or reject client, and check login    //
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pwd.h>
#include <errno.h>

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
    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}

    ////// set server address for bind //////
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(atoi(argv[1]));

    ///// bind server address with server socket /////
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		exit(1);
	}

    listen(listenfd, 5); // listen queue size: 5
    while (1)
    {
        int clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) /// accept client's connection ///
		{
			perror("accept");
			exit(1);
		}
		printf("** Client is connected **\n"); // success to connect
		printf(" - IP: %s\n - Port: %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

		if((fp_checkIP = fopen("access.txt", "r")) == NULL) // open access.txt file and check error
			exit(1);
		
		char buf[MAX_BUF]; // store a line from 'access.txt'
		char cli_ip[MAX_BUF];
		strcpy(cli_ip, inet_ntoa(cliaddr.sin_addr)); // store client ip to cli_ip

		char *cli_ips[5]; // split client ip address by '.'
		cli_ips[0] = strtok(cli_ip, ".");
		for (int i = 1; (cli_ips[i] = strtok(NULL, ".")) != NULL; i++);
		
		char *ptr, *tmp; // ptr -> check incorrect ip address, tmp -> splitted buf
		while ((ptr = fgets(buf, MAX_BUF, fp_checkIP)) != NULL)
		{
			tmp = strtok(buf, ".");
			if (!tmp || (strcmp(tmp, cli_ips[0]) && strcmp(tmp, "*"))) // first Byte dismatch
				continue;
			tmp = strtok(NULL, ".");
			if (!tmp || (strcmp(tmp, cli_ips[1]) && strcmp(tmp, "*"))) // second Byte dismatch
				continue;
			tmp = strtok(NULL, ".");
			if (!tmp || (strcmp(tmp, cli_ips[2]) && strcmp(tmp, "*"))) // third Byte dismatch
				continue;
			tmp = strtok(NULL, ".");
			if (!tmp || (strcmp(tmp, cli_ips[3]) && strcmp(tmp, "*"))) // last Byte dismatch
				continue;
			break; // if match with ip in 'access.txt', stop compare
		}
		fclose(fp_checkIP); // no longer use
		if (!ptr) // escape while() because of no matching
		{
			printf("** It is NOT authenticated client **\n");
			write(connfd, "REJECTION", MAX_BUF);
			close(connfd);
			continue;
		}
		else
			write(connfd, "ACCEPTED", MAX_BUF); // match

        if (log_auth(connfd) == 0) // if login failed
        {
            printf("** Fail to log-in **\n");
            close(connfd);
            continue;
        }
        printf("** Success to log-in **\n"); // success to login
        close(connfd);
    }
}

////////////////////////////////////////////////////////////////////////
// log_auth                                                           //
// ================================================================== //
// Input: int -> socket descriptor that is used for client connection //
//                                                                    //
// Output: int -> 0: failed to login                                  //
//                1: success to login                                 //
// Purpose: compare user name(ID), passwd read from client with       //
//	        ID & passwd in the file 'passwd'                          //
////////////////////////////////////////////////////////////////////////

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
		passwd[n] = '\0'; // read user name and passwd from client
        write(connfd, "OK", MAX_BUF); // read well from client

		printf("** User is trying to log-in (%d/3) **\n", count); // print login try count

        if ((n = user_match(user, passwd)) == 1) // if user_match succeed, write OK to client
		{
			write(connfd, "OK", MAX_BUF);
			break;
		}
        else if (n == 0) // failed to login
        {
			printf("** Log-in failed **\n"); // print failures
            if (count >= 3) // if failed more than 3 times, stop login
            {
				write(connfd, "DISCONNECTION", MAX_BUF);
				return 0;
            }
            write(connfd, "FAIL", MAX_BUF); // failures less than 3 times, send 'FAIL' to client 
            count++;
            continue;
        }
    }
    return 1; // return success
}

////////////////////////////////////////////////////////////////////////
// user_match                                                         //
// ================================================================== //
// Input: char * -> user name                                         //
//		  char * -> passwd                                            //
//                                                                    //
// Output: int -> 0 : there's no ID & passwd in 'passwd'              //
//                1 : ID & passwd exist in 'passwd'                   //
// Purpose: compare ID & passwd(parameter) with ID & passwd in passwd //                       
////////////////////////////////////////////////////////////////////////

int user_match(char *user, char *passwd)
{
    FILE *fp;
    struct passwd *pw;

    fp = fopen("passwd", "r"); // open passwd file stream for read

	while ((pw = fgetpwent(fp)) != NULL) // parsing passwd line(ID:passwd:uid:gid:...) and store to struct passwd
	{
		if (!strcmp(user, pw->pw_name) && !strcmp(passwd, pw->pw_passwd)) // correspond with oasswd
			return 1; // success
	}
	fclose(fp); // close file stream
	return 0;
}