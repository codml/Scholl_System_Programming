////////////////////////////////////////////////////////////////////////
// File Name    :cli.c                                                //
// Date         :2024/05/20                                           //
// OS           :Ubuntu 20.04.6 LTS 64bits                            //
// Author       :Kim Tae Wan                                          //
// Student ID   :2020202034                                           //
// ------------------------------------------------------------------ //
// Title        :System Programming Assignment #3-1: server with login//
// Description  :client - access to server and login with ID & passwd //
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

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
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror(strerror(errno));
		exit(1);
	}

	////// set server address structure(including information) //////
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	servaddr.sin_port=htons(atoi(argv[2]));

	///// connect to server + error handling /////
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect");
		exit(1);
	}

    log_in(sockfd); /// check rejection & acception, read ID & passwd and send them, then receive response ///
    close(sockfd); /// close connection ///
    return 0;
}

////////////////////////////////////////////////////////////////////////
// log_in                                                             //
// ================================================================== //
// Input: int -> socket descriptor that is used for server connection //
//                                                                    //
// Output: none                                                       //
//                                                                    //
// Purpose: check connection to server, send ID & passwd and receive  //
//				answer from server                                    //
////////////////////////////////////////////////////////////////////////

void log_in(int sockfd)
{
    int n;
    char user[MAX_BUF], *passwd, buf[MAX_BUF];

	// read REJECTION or ACCEPTED -> ip access //
    if ((n = read(sockfd, buf, MAX_BUF)) <= 0)
	{
		perror("Read error");
		exit(1);
	}
    buf[n] = '\0';
    if (!strcmp(buf, "REJECTION")) // rejected from server
    {
		printf("** Connection refused ""\n");
		close(sockfd);
		exit(1);
	}
	else // receive ACCEPTED
		printf("** It is connected to Server **\n");

    while (1) // read ID & passwd from STDIN_FILENO and send to server, then receive OK, FAIL, or DISCONNECTION //
    {
		write(STDOUT_FILENO, "Input ID : ", strlen("Input ID : "));
		if ((n = read(STDIN_FILENO, buf, MAX_BUF)) <= 0) // if read failed, exit
			exit(1);
		buf[n - 1] = '\0'; // store input except line feed('\n')
		if (write(sockfd, buf, MAX_BUF) <= 0) // write ID to server
			exit(0);

		strcpy(user, buf); // store user name(ID) to print result

		passwd = getpass("Input passwd : "); // receive passwd encrypted
		if (write(sockfd, passwd, MAX_BUF) <= 0) // write to server
			exit(1);

		if ((n = read(sockfd, buf, MAX_BUF)) <= 0) // receive result from server
			exit(1);
		buf[n] = '\0';
        if (!strcmp(buf, "OK")) // ID, passwd arrived well
        {
            if ((n = read(sockfd, buf, MAX_BUF)) <= 0) // read OK, or FAIL, or DISCONNECTION
				exit(1);
            buf[n] = '\0';

            if (!strcmp(buf, "OK")) // login success
			{
				printf("** User '[%s]' logged in **\n", user);
				break;
			}	
            else if (!strcmp(buf, "FAIL")) // fail less than 3 times
				printf("** Log-in failed **\n");
            else
			{
				printf("** Connection closed **\n"); // failures during 3 times
				break;
			}
				
        }
    }
}