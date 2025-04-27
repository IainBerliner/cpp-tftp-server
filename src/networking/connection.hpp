#pragma once

#include "networking.hpp"
#include "../macros/constants.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

class connection {
    std::string port;
    std::shared_ptr<int> sockfd_ptr;
    struct sockaddr_storage conn_addr;
    socklen_t addr_len = sizeof conn_addr;
    //Skip receive/receive_timeout on first access because buffer will already be filled
    bool first_access = true;
    std::vector<unsigned char> buffer;
    int numbytes;
    static void close_sockfd_ptr(int* sockfd_ptr);
public:
    ~connection() { }
    connection () {
        sockfd_ptr = std::shared_ptr<int>(new int, connection::close_sockfd_ptr); //Set custom deleter
        *sockfd_ptr = -1;
    }
    connection (int32_t port_num) {
        struct addrinfo hints;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;    //Use IPV4 or IPV6
        hints.ai_socktype = SOCK_DGRAM; //Use UDP protocol
        hints.ai_flags = AI_PASSIVE;    //Use my IP
        sockfd_ptr = std::shared_ptr<int>(new int, connection::close_sockfd_ptr); //Set custom deleter
        port = std::to_string(port_num);
        *sockfd_ptr = get_bound_socket(NULL, port.c_str(), hints);
        #ifdef DEBUG_BUILD
        std::cout << "connection::connection(int32_t port_num): *sockfd_ptr: ";
        std::cout << *sockfd_ptr << std::endl;
        #endif
    }
    int listen(int timeout_sec, int timeout_usec);
    int receive();
    int receive_timeout(int timeout_sec, int timeout_usec);
    std::vector<unsigned char>& get_buffer();
    bool send(const std::vector<unsigned char> packet);
    bool valid();
};
