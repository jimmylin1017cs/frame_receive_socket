#ifndef SOCKET_HEADER_H
#define SOCKET_HEADER_H

#include <stdio.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h> // SIGPIPE

#include <netdb.h>
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // convert ip

#define PORT unsigned short
#define SOCKET int
#define HOSTENT struct hostent
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define ADDRPOINTER unsigned int *
#define SOCKLEN_T socklen_t
#define SELECT_SET fd_set
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#define BUFFER_MAX 4096

#endif