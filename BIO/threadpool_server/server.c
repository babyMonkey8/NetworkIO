/* server：作用是接收客户端发送过来的字符，然后将每个字符转换为大写并回送给客户端。 */
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include "wrap.h"
#include "threadpoolsimple.h"




#define SERV_PORT 6666

void *do_work(void *arg)
{
	int n,i;
	struct s_info *ts = (struct s_info*)arg;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	/* 可以在创建线程前设置线程创建属性,设为分离态,哪种效率高呢？ */
	pthread_detach(pthread_self());
	while (1) 
	{
		n = Read(ts->connfd, buf, MAXLINE);
		if(n < 0)
		{
			perror("Read Error");
			close(ts->connfd);
			exit(0);
		}
		if (n == 0) 
		{
			printf("the other side has been closed.\n");
			close(ts->connfd);
			break;
		}
		printf("received from %s at PORT %d\n",
				inet_ntop(AF_INET, &(*ts).cliaddr.sin_addr, str, sizeof(str)),
				ntohs((*ts).cliaddr.sin_port));
		for (i = 0; i < n; i++)
			buf[i] = toupper(buf[i]);
		Write(ts->connfd, buf, n);
	}
	Close(ts->connfd);
}

int main(void)
{
	create_threadpool(3,20);

	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddr_len;
	int listenfd, connfd;
	
	struct s_info ts[256];

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
		struct s_info ts;

		// 提取连接
		cliaddr_len = sizeof(cliaddr);
		connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
		ts.cliaddr = cliaddr;
		ts.connfd = connfd;
		addtask(thrPool, (void*)&ts);//模拟添加任务
	}

	destroy_threadpool(thrPool);
	return 0;
}
// gcc server.c threadpoolsimple.c wrap.c -o server -lpthread