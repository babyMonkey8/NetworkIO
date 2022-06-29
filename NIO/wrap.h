#ifndef __WRAP_H_
#define __WRAP_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>

void perr_exit(const char *s);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Listen(int fd, int backlog);
int Socket(int family, int type, int protocol);
ssize_t Read(int fd, void *ptr, size_t nbytes);
ssize_t Write(int fd, const void *ptr, size_t nbytes);
int Close(int fd);

ssize_t Readn(int fd, void *vptr, size_t n);                            // 读取固定长度的数据
ssize_t Writen(int fd, const void *vptr, size_t n);                     // 写入固定长度的数据
ssize_t my_read(int fd, char *ptr);                                     // 读取一个字节数据
ssize_t Readline(int fd, void *vptr, size_t maxlen);                    // 读取固一行数据
int tcp4bind(short port,const char *IP);                                // 创建套接字并绑定
#endif
