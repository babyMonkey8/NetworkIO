/* server：作用是接收客户端发送过来的字符，然后将每个字符转换为大写并回送给客户端。 */
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include "wrap.h"

#define MAXLINE 80
#define SERV_PORT 6666

void do_sigchild(int num)
{
	while (waitpid(0, NULL, WNOHANG) > 0)
    ;
}

int main(void)
{
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddr_len;
	int listenfd, connfd;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	int i, n;
	pid_t pid;

	struct sigaction newact;
	newact.sa_handler = do_sigchild;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags = 0;
	sigaction(SIGCHLD, &newact, NULL);

	// 创建套接字
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	// 绑定套接字
	Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// 监听
	Listen(listenfd, 20);

	printf("Accepting connections ...\n");
	while (1) 
	{
		// 提取连接
		cliaddr_len = sizeof(cliaddr);
		connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);

		pid = fork();
		if (pid == 0) 				//子进程
		{
			Close(listenfd);
			while (1) 
			{
				n = Read(connfd, buf, MAXLINE);
				if(n < 0)
				{
					perror("Read Error");
					close(connfd);
					exit(0);
				}
				if (n == 0) 										
				{
					printf("the other side has been closed.\n");
					close(connfd);
					break;
				}
				printf("received from %s at PORT %d\n",
						inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
						ntohs(cliaddr.sin_port));
				for (i = 0; i < n; i++)
					buf[i] = toupper(buf[i]);
				Write(connfd, buf, n);
			}
			Close(connfd);
			return 0;
		} 
		else if (pid > 0)		// 父进程
		{
			Close(connfd);
		} 
		else
		{
			perr_exit("fork error");
		}	
	}
	Close(listenfd);
	return 0;
}