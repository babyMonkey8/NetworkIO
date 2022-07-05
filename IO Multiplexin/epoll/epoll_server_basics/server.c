/* server：作用是接收客户端发送过来的字符，然后将每个字符转换为大写并回送给客户端。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctype.h>
#include "wrap.h"

#define MAXLINE 80
#define SERV_PORT 6666
#define OPEN_MAX 1024

int main(int argc, char *argv[])
{
	int i, j, maxi, listenfd, connfd, sockfd;
	int nready, efd, res;
	ssize_t n;
	char buf[MAXLINE], str[INET_ADDRSTRLEN];
	socklen_t clilen;
	int client[OPEN_MAX];
	struct sockaddr_in cliaddr, servaddr;
	struct epoll_event tep, ep[OPEN_MAX];

	// 创建套接字
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	// 绑定套接字
	Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	// 监听
	Listen(listenfd, 20);

	for (i = 0; i < OPEN_MAX; i++)
		client[i] = -1;
	maxi = -1;

	//创建树
	efd = epoll_create(OPEN_MAX);
	if (efd == -1)
		perr_exit("epoll_create");

	tep.events = EPOLLIN; tep.data.fd = listenfd;

	// 上树
	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
	if (res == -1)
		perr_exit("epoll_ctl");

	printf("Accepting connections ...\n");

	// 监听
	while (1) {
		nready = epoll_wait(efd, ep, OPEN_MAX, -1); /* 阻塞监听 */
		if (nready == -1)
		{
			perr_exit("epoll_wait");
			break;
		}
			
		if( nready == 0)
		{
			continue;
		}
		

		for (i = 0; i < nready; i++) {
			//判断 listenfd 变化,并且是读事件变化	
			if (ep[i].data.fd == listenfd && ep[i].events & EPOLLIN) {
				clilen = sizeof(cliaddr);

				//提取新的连接 
				connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
				printf("received from %s at PORT %d\n", 
						inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), 
						ntohs(cliaddr.sin_port));
				for (j = 0; j < OPEN_MAX; j++) {
					if (client[j] < 0) {
						client[j] = connfd; /* save descriptor */
						break;
					}
				}

				if (j == OPEN_MAX)
					perr_exit("too many clients");
				if (j > maxi)
					maxi = j; 		/* max index in client[] array */

				tep.events = EPOLLIN; 
				tep.data.fd = connfd;
				//将connfd上树
				res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
				if (res == -1)
					perr_exit("epoll_ctl");
			} 
			else if (ep[i].events & EPOLLIN) //connfd变化 ,而且是读事件变化
			{
				sockfd = ep[i].data.fd;
				n = Read(sockfd, buf, MAXLINE);
				if (n == 0) {
					for (j = 0; j <= maxi; j++) {
						if (client[j] == sockfd) {
							client[j] = -1;
							break;
						}
					}
					//下树
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
					if (res == -1)
						perr_exit("epoll_ctl");

					Close(sockfd);
					printf("client[%d] closed connection\n", j);
				} else {
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					Writen(sockfd, buf, n);
				}
			}
		}
	}
	close(listenfd);
	close(efd);
	return 0;
}