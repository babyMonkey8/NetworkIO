/* server：作用是接收客户端发送过来的字符，然后将每个字符转换为大写并回送给客户端。 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "wrap.h"

#define MAXLINE 80
#define SERV_PORT 6666

int main(int argc, char *argv[])
{
	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE]; 	/* FD_SETSIZE 默认为 1024 */
	ssize_t n;
	fd_set rset, allset;				/* reset 表示当前集合；allset表示旧的集合 */
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN]; 			/* #define INET_ADDRSTRLEN 16 */
	socklen_t cliaddr_len;
	struct sockaddr_in cliaddr, servaddr;

	// 创建套接字
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	// 绑定套接字
	Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	// 监听
	Listen(listenfd, 20); 		/* 默认最大128 */

	maxfd = listenfd; 			/* 初始化最大的文件描述符 */  
	maxi = -1;					/* client[]的下标 */

	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1; 		/* 用-1初始化client[] */

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset); /* 构造select监控文件描述符集 */

	printf("Accepting connections ...\n");

	for ( ; ; ) 
	{
		rset = allset; 			/* 每次循环时都从新设置select监控信号集，即将将旧的信号集赋值给需要监听的信号集 */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if (nready < 0)	
			perr_exit("select error");

		//listenfd 变化 代表有新的连接到来
		if (FD_ISSET(listenfd, &rset)) 
		{
			// 提取连接
			cliaddr_len = sizeof(cliaddr);
			connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
			printf("received from %s at PORT %d\n",
					inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
					ntohs(cliaddr.sin_port));
			for (i = 0; i < FD_SETSIZE; i++) 
			{
				if (client[i] < 0) 
				{
					client[i] = connfd; /* 保存accept返回的文件描述符到client[]里 */
					break;
				}
			}
			
			/* 达到select能监控的文件个数上限 1024 */
			if (i == FD_SETSIZE) 
			{
				fputs("too many clients\n", stderr);
				exit(1);
			}

			FD_SET(connfd, &allset); 	/* 添加一个新的文件描述符到监控信号集(旧的集合)里，以便下次监听 */
			if (connfd > maxfd)
				maxfd = connfd; 		/* select第一个参数需要 */
			if (i > maxi)
				maxi = i; 				/* 更新client[]最大下标值 */

			if (--nready == 0)
				continue; 				/* 如果没有更多的就绪文件描述符继续回到上面select阻塞监听,
											负责处理未处理完的就绪文件描述符 */
		}

		// 遍历 listenfd（位图位置） 之后的文件描述符是否在当前集合中,如果在则 cfd 变化
		for (i = 0; i <= maxi; i++) 
		{ 	/* 检测哪个clients 有数据就绪 */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) 
			{
				int n = Read(sockfd, buf, MAXLINE);
				if (n < 0)				/* 读出错,将cfd关闭,从当前集合中删除cfd*/
				{
					perror("read error");
					Close(sockfd);
					FD_CLR(sockfd, &rset);		
				}
				if ( n == 0) 
				{
					Close(sockfd);		/* 当client关闭链接时，服务器端也关闭对应链接 */
					FD_CLR(sockfd, &allset); /* 解除select监控此文件描述符 */
					client[i] = -1;
				} else 
				{
					int j;
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					Write(sockfd, buf, n);
				}
				if (--nready == 0)
					break;
			}
		}
	}
	close(listenfd);
	return 0;
}