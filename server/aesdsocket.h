/*
 * =====================================================================================
 *
 *       Filename:  aesdsocket.h
 *
 *    Description:  Assignment 5 part 1
 *
 *        Version:  1.0
 *        Created:  March 17, 2024.
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Youssef Essam 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>


#ifndef AESD_SOCKET
#define AESD_SOCKET

#define MYPORT          9000
#define LISTEN_BACKLOG  5
#define BUFFER_SIZE     (1024*30)

#define ERROR_HANDLER(func) { perror(#func); exit(EXIT_FAILURE); }
//#define USER_LOGGING(format, addr, port) syslog(LOG_DEBUG, format, addr, port)
//#define USER_ERRLOG(format, addr) syslog(LOG_ERR, format, addr)
//#define USER_DEBLOG(format, addr) syslog(LOG_DEBUG, format, addr)

#define USER_LOGGING(format, addr)          syslog(LOG_DEBUG, format, addr)
//#define USER_SIGNAL_LOGGING(format, addr)   syslog(LOG_DEBUG, format)

extern int signal_sign;

/* A structure contains network fd and file fd. */
struct fdset {
    int sock; 
    int fd;
};

int tcp_socket_setup(int domain, int type, int protocol, struct sockaddr_in addr, int reuse);
int tcp_incoming_check(int sockfd, struct sockaddr_in *client, socklen_t addrlen);
void tcp_shutdown(int sockfd, int how);
void tcp_close (int sockfd);
int tcp_receive(int acceptfd, char *buffer, int size);
int tcp_send(int acceptfd, char *buffer, int size);
void *tcp_echoback (void *arg);
void tcp_set_nonblock(int sockfd, int invert);
int tcp_select(int sockfd);
int tcp_getopt(int argc, char *argv[]);
void process_kill(int sockfd, struct fdset * tcpfd);


#endif 