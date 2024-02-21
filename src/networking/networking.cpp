#include "networking.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../handle_connection/handle_connection.hpp"
#include "../macros/constants.hpp"
#include "../macros/pthread_macros.hpp"

// get sockport, IPv4 or IPv6:
unsigned short int get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return ((struct sockaddr_in*)sa)->sin_port;
    }

    return ((struct sockaddr_in6*)sa)->sin6_port;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int get_bound_socket(const char* address, const char* port, struct addrinfo hints) {
    int sockfd;
    int rv;
    #ifdef TESTING_BUILD
    printf("get_bound_socket: getting bound socket.\n");
    #endif
    struct addrinfo *servinfo, *p;
    int yes=1;
    if ((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "get_bound_socket: %s\n", gai_strerror(rv));
        pthread_exit(NULL);
    }
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("server: setsockopt");
            pthread_exit(NULL);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }
    
    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "get_bound_socket: failed to bind\n");
        pthread_exit(NULL);
    }
    
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, p->ai_addr, str, INET_ADDRSTRLEN);
    printf("server: the address: %s\n", str);
    
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    getsockname(sockfd, (struct sockaddr *)&sin, &len);
    printf("server: the port: %d\n", ntohs(sin.sin_port));
    
    return sockfd;
}

//Will listen to incoming connections on a specified port and direct all of them to the input sockfd_thread_function.
#ifndef TESTING_BUILD
int tftp_listen(const char* port, const char* tftp_root, const char* permitted_operations, const bool allow_leaving_tftp_root, std::shared_ptr<std::atomic<bool>> running) {
#else
int tftp_listen(const char* port, const char* tftp_root, const char* permitted_operations, const bool allow_leaving_tftp_root,
		const float drop, const float duplicate, const float bitflip, std::shared_ptr<std::atomic<bool>> running) {
#endif
    THREAD_START()
    
    //thread_local std::shared_ptr<std::atomic<bool>> running = input_running;
    
    THREAD_REGISTER_CLEANUP(NOTIFY_CHILDREN_OF_DEATH, running)
    
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    //struct addrinfo hints, *servinfo, *p;
    struct addrinfo hints;
    //int rv;
    int numbytes;
    struct sockaddr_storage new_addr; // connector's address information
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    //char s[INET6_ADDRSTRLEN];
    int yes=1;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    sockfd = get_bound_socket(NULL, port, hints);
    
    THREAD_REGISTER_CLEANUP(CLOSE_SOCKFD, sockfd)
    
    while (1) {  // main recvfrom() loop
    
        addr_len = sizeof new_addr;
        
        THREAD_SPECIFY_CANCELLATION_POINT(
            if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&new_addr, &addr_len)) == -1) {
                perror("recvfrom");
                pthread_exit(NULL);
                continue;
            }
        )
        
        int new_sockfd;
        if ((new_sockfd = get_bound_socket(NULL, port, hints)) == -1) {
            perror("get_bound_socket");
            continue;
        }
        connect(new_sockfd, (struct sockaddr *)&new_addr, addr_len);
        
        handle_connection_input_struct* param = new handle_connection_input_struct();
        param->sockfd = new_sockfd;
        memcpy(param->buf, buf, MAXBUFLEN);
        param->numbytes = numbytes;
        param->tftp_root = tftp_root;
        param->permitted_operations = permitted_operations;
        param->allow_leaving_tftp_root = allow_leaving_tftp_root;
        #ifdef TESTING_BUILD
        param->drop = drop;
        param->duplicate = duplicate;
        param->bitflip = bitflip;
        #endif
        param->parent_thread_alive = running;
        
        pthread_t new_thread;
        
        if(pthread_create(&new_thread, NULL, handle_connection, param)){
            perror("ERROR creating thread.");
        }
        
    }
    
    //Cleanup everything (such as the sockfd)
    THREAD_CLEANUP()
    
    pthread_exit(NULL);
}

int recvtimeout(int s, char *buf, int len, int flags, int timeout_sec, int timeout_usec) {
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


