/*
 * =====================================================================================
 *
 *       Filename:  aesdsocket.c
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include "aesdsocket.h"
#include "manipulate_file.h" 
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>



/* Global variable */
int signal_sign = 0;

/* function prototype */
int signal_setup(int, ...);

int tcp_socket_setup(int domain, int type, int protocol, struct sockaddr_in addr, int reuse) {

    int sockfd, rc, on = 1;

    printf("Initializing the socket ....\n");

    sockfd = socket(domain, type, protocol);
    if (sockfd == -1) {
        ERROR_HANDLER(socket);
    }

    if (reuse == true) {
        // Set socket option to enable reuse the local address
        rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, (socklen_t) sizeof(on));
        if (rc == -1) {
            ERROR_HANDLER(setsockopt);
        }
    }

    rc = bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr));
    if (rc == -1) {
        ERROR_HANDLER(bind);
    }

    return sockfd;
}

int tcp_incoming_check(int sockfd, struct sockaddr_in *client, socklen_t addrlen) {

    int rc, acceptfd;

    rc = listen(sockfd, 5);
    if (rc == -1) {
        ERROR_HANDLER(listen);
    }
  
    rc = tcp_select(sockfd);
    if (rc == -1) {
        printf("tcp_select no pick up.\n");
        return -1;
    } else if (rc == 0) {
        printf("select timeout.\n");
        return 0;
    }

    acceptfd = accept(sockfd, (struct sockaddr *) client, &addrlen);
    if (acceptfd == -1) {
        ERROR_HANDLER(accept);
    }
    return acceptfd;
}

void tcp_shutdown(int sockfd, int how) {

    if (shutdown(sockfd, how)) {
        ERROR_HANDLER(shutdown);
    }
    return;
}

void tcp_close (int sockfd) {

    if (sockfd > 0) {
        tcp_shutdown(sockfd, SHUT_RDWR);
    }
    if (close(sockfd)) {
        ERROR_HANDLER(close);
    }
    return;
}

int tcp_receive(int acceptfd, char *buffer, int size) {


    int rc;
    rc = tcp_select(acceptfd);
    rc = recv(acceptfd, buffer, size, 0); 

    if (rc < 0) {
        ERROR_HANDLER(recv);
    } else if (rc == 0) {
        USER_LOGGING("The client-side might was closed, rc: %d\n", rc);
    }
    return rc;
}


int tcp_send(int acceptfd, char *buffer, int size) {

    int rc;
    rc = send(acceptfd, buffer, size, 0);
    if (rc != size) {
        ERROR_HANDLER(send);
    }
    return rc;
}

void *tcp_echoback (void *arg) {

    int rc = 0;
    char sbuffer[BUFFER_SIZE] = {0}, rbuffer[BUFFER_SIZE] = {0};
    struct fdset *tcpfd = (struct fdset *) arg;

    do {
        rc = tcp_receive(tcpfd->sock, rbuffer, sizeof(rbuffer));
        if (rc > 0) {
            if (strchr(rbuffer, '\n')) {
                //fprintf(stdout, "receive: %s", rbuffer);
                rc = file_write(tcpfd->fd, rbuffer, rc);
                fsync(tcpfd->fd);
                rc = file_read(tcpfd->fd, sbuffer, file_size(tcpfd->fd));
                fsync(tcpfd->fd);
                tcp_send(tcpfd->sock, sbuffer, strlen(sbuffer));
                lseek(tcpfd->fd, 0, SEEK_SET);
                memset(rbuffer, 0, sizeof(rbuffer));
                memset(sbuffer, 0, sizeof(sbuffer));
            }
        } else if (rc <= 0) {
            break;
        }
    } while (1);

    return NULL;
}

void tcp_set_nonblock(int sockfd, int invert) {

    int flag;

    flag = fcntl(sockfd, F_GETFL);
    if (!invert) {
        fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
    } else {
        fcntl(sockfd, F_SETFL, flag ^ O_NONBLOCK);
    }

    return;
}

int tcp_select(int sockfd)
{
    int rc;
    fd_set readfds;
    struct timeval time = {0, 0};
    time.tv_sec = 60;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    rc = select(sockfd + 1, &readfds, NULL, NULL, &time);

    if (rc > 0 && FD_ISSET(sockfd, &readfds)) {
        return sockfd;
    } else if (rc == 0) {
        return 0;
    } else {
        // Handle error from select() (rc == -1)
        perror("select");
        return -1;
    }
}

int tcp_getopt(int argc, char *argv[]) {

    int ch, rc;

    while ((ch = getopt(argc, argv, "d")) != -1) {
        switch(ch) {
            case 'd':
                printf("Convert %s into daemon.\n", __FILE__);
                rc = daemon(1, 0);
                USER_LOGGING("Running as daemon: %d\n", rc);
                return true;
        }
    }
    return false;
} 

void process_kill(int sockfd, struct fdset * tcpfd) {
    
    if(tcpfd != NULL) {
        tcp_close(tcpfd->sock);
        close(tcpfd->fd);
        free(tcpfd);
    }
        
    tcp_close(sockfd);
    closelog();
}


int main(int argc, char *argv[]) {

    int sockfd, acceptfd, rc, datafd;
    pid_t id = 0;
    pthread_t pid;
    struct sockaddr_in serv, client;
    struct fdset *tcpfd = NULL;
    socklen_t addrlen;
    
    printf("Welcom to socket Testing program\n");
    
    // initialize the serv struct of my socket
    memset(&serv, 0 ,sizeof(struct sockaddr));
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(9000);
    serv.sin_family = AF_INET;
    tcpfd = (struct fdset *) malloc(sizeof(struct fdset));

    // Set syslog configuration up.
    openlog(NULL, LOG_PID, LOG_USER);

    // Create a file to write the data received from a client.
    file_delete("/var/tmp/aesdsocketdata");
    datafd = file_create("/var/tmp/aesdsocketdata", 2, O_RDWR, O_CREAT);
    tcpfd->fd = datafd;

    sockfd = tcp_socket_setup(AF_INET, SOCK_STREAM, IPPROTO_TCP, serv, true);
    if (sockfd == -1) {
        process_kill(sockfd, tcpfd);
        ERROR_HANDLER(socket);
    }
    tcp_set_nonblock(sockfd, 0);

    rc = tcp_getopt(argc, argv);
    if (rc == false) {
        printf("No any options.\n");
    }

    if (id != 0) {
        perror("daemon ");
        exit(EXIT_SUCCESS);
    }

    rc = signal_setup(2, SIGINT, SIGTERM);
    //tcp_set_nonblock(sockfd, 1);

    while (1) {

        addrlen = sizeof(struct sockaddr);
        acceptfd = tcp_incoming_check(sockfd, &client, addrlen);
        if (acceptfd > 0) {
            //printf("Accepted connection from %s:%d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            USER_LOGGING("Accepted connection from %s", inet_ntoa(client.sin_addr));
            tcpfd->sock = acceptfd;

            rc = pthread_create(&pid, NULL, tcp_echoback, (void *) tcpfd);
            if (rc == -1) {
                process_kill(sockfd, tcpfd);
                ERROR_HANDLER(pthread_create);
            }

            rc = pthread_join(pid, NULL);
            if (rc == -1) {
                process_kill(sockfd, tcpfd);
                ERROR_HANDLER(pthread_create);
            }

        }

        if(signal_sign)
        {
            file_delete("/var/tmp/aesdsocketdata");
            process_kill(sockfd, tcpfd);
            break;
        }
        if(acceptfd == 0)
        {
            USER_LOGGING("Closed connection from %s", inet_ntoa(client.sin_addr));
            tcp_close(acceptfd);

            sleep(1);
            printf("Waiting for new connection request or SIGINT or SIGTERM to kill the process.\n");

        }        
    }
    
    /*tcp_close(acceptfd);
    tcp_close(sockfd);
    close(tcpfd->fd);
    free(tcpfd);
    closelog();*/

    return 0;
}