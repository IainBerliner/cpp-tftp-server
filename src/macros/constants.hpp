#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP
#define BACKLOG 10   // how many pending connections queue will hold
#define MAXBUFLEN 65468 // the maximum UDP packet size this server needs to be able to hold
#define MAX_U_NETWORK_SHORT_SIZE 65536 //needed to "roll over" and start back at data transfer block number 1 when the ordinary TFTP file transfer limit is hit
#define ARBITRARY_PATH_SIZE_LIMIT 10000
#endif
