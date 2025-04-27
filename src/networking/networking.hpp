#pragma once

#include <atomic>
#include <memory>

int get_bound_socket(const char* address, const char* port, struct addrinfo hints);
int recvtimeout(int s, unsigned char *buf, int len, int flags, int timeout_sec, int timeout_usec);
int recvfromtimeout(int s, unsigned char *buf, int len, int flags, struct sockaddr* conn_addr, socklen_t* addr_len, int timeout_sec, int timeout_usec);
