#include "networking.hpp"

#include "../macros/constants.hpp"

#include <atomic>
#include <iostream>
#include <memory>

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

//Get sockport, IPv4 or IPv6.
unsigned short int get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return ((struct sockaddr_in*)sa)->sin_port;
    }

    return ((struct sockaddr_in6*)sa)->sin6_port;
}

//Get sockaddr, IPv4 or IPv6.
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Exactly what it says on the tin.
int get_bound_socket(const char* address, const char* port, struct addrinfo hints) {
    int sockfd;
    int rv;
    struct addrinfo *servinfo, *p;
    int yes=1;
    
    if ((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0) {
        #ifdef DEBUG_BUILD
        std::cout << "get_bound_socket: " << gai_strerror(rv) << std::endl;
        #endif
        return -1;
    }
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            #ifdef DEBUG_BUILD
            std::cout << "get_bound_socket: socket: " << strerror(errno) << std::endl;
            #endif
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            #ifdef DEBUG_BUILD
            std::cout << "get_bound_socket: setsockopt: " << strerror(errno) << std::endl;
            #endif
            return -1;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            #ifdef DEBUG_BUILD
            std::cout << "get_bound_socket: bind: " << strerror(errno) << std::endl;
            #endif
            continue;
        }

        break;
    }
    
    freeaddrinfo(servinfo);

    if (p == NULL)  {
        std::cerr << "get_bound_socket: failed to bind." << std::endl;
        return -1;
    }
    
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, p->ai_addr, str, INET_ADDRSTRLEN);
    
    //make sure port number is correct
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    getsockname(sockfd, (struct sockaddr *)&sin, &len);
    if (ntohs(sin.sin_port) != std::atoi(port)) {
        std::cout << "get_bound_socket: port " << port << " is in use." << std::endl;
        close(sockfd); 
        return -1;
    }
    
    return sockfd;
}

/*
//Like recv, but return -2 if time specified in timeout_sec (seconds) and timeout_usec
//(microseconds) is exceeded.
*/
int recvtimeout(int s, unsigned char *buf, int len, int flags, int timeout_sec, int timeout_usec) {
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout_sec;
    tv.tv_usec = timeout_usec;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return recv(s, buf, len, flags);
}

//Like recvtimeout, but allows the caller to obtain the new connector's address information.
int recvfromtimeout(int s, unsigned char *buf, int len, int flags, struct sockaddr* conn_addr,
                    socklen_t* addr_len, int timeout_sec, int timeout_usec) {
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout_sec;
    tv.tv_usec = timeout_usec;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recvfrom()
    return recvfrom(s, buf, len , flags, conn_addr, addr_len);
}
