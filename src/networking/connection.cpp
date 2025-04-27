#include "connection.hpp"

#include <iostream>

void connection::close_sockfd_ptr(int* sockfd_ptr) {
    if (*sockfd_ptr != -1) {
        #ifdef DEBUG_BUILD
        std::cout << "connection::close_sockfd_ptr: *sockfd_ptr: " << *sockfd_ptr << std::endl;
        #endif
        close(*sockfd_ptr);
    }
    delete sockfd_ptr;
    return;
}

int connection::listen(int timeout_sec, int timeout_usec) {
    buffer.resize(MAX_UDP_PACKET);
    numbytes = recvfromtimeout(*sockfd_ptr, buffer.data(), MAX_UDP_PACKET, 0,
                               (struct sockaddr *)&conn_addr, &addr_len, timeout_sec, timeout_usec);
    if (numbytes == -1) {
        perror("recvfrom");
        buffer.resize(0);
        return -1;
    }
    if (numbytes == -2) {
        buffer.resize(0);
        return -2;
    }
    buffer.resize(numbytes);
    if (connect(*sockfd_ptr, (struct sockaddr *)&conn_addr, addr_len) == -1) {
        perror("connect");
        return -1;
    }
    return numbytes;
}

int connection::receive() {
    if (first_access) { first_access = false; return 0; };
    buffer.resize(MAX_UDP_PACKET);
    if ((numbytes = recv(*sockfd_ptr, buffer.data(), MAX_UDP_PACKET, 0) == -1)) {
        perror("recv");
        buffer.resize(0);
        return numbytes;
    }
    buffer.resize(numbytes);
    return numbytes;
}

int connection::receive_timeout(int timeout_sec, int timeout_usec) {
    if (first_access) { first_access = false; return 0; };
    buffer.resize(MAX_UDP_PACKET);
    if ((numbytes=recvtimeout(*sockfd_ptr,buffer.data(),MAX_UDP_PACKET,0,timeout_sec,timeout_usec))<=-1) {
        perror("recvtimeout");
        buffer.resize(0);
        return numbytes;
    }
    buffer.resize(numbytes);
    return numbytes;
}

std::vector<unsigned char>& connection::get_buffer() {
    return this->buffer;
}

bool connection::send(const std::vector<unsigned char> packet) {
    return (::send(*sockfd_ptr, (void*)packet.data(), packet.size(), 0) != -1);
}

bool connection::valid() {
    return (*sockfd_ptr != -1);
}
