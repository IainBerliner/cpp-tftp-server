#pragma once

/*
//The maximum UDP packet size the server can receive (deliberate slight overestimate because this
varies by platform).
*/
#define MAX_UDP_PACKET 65535

/*
//Needed to "roll over" and start back at data transfer block number 1 when the ordinary TFTP file
//transfer limit is hit
*/
#define BLOCK_NUMBER_LIMIT 65536

//Opcodes of various TFTP operations
#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5
#define OP_OACK 6

//The different modes the server can be running in
#define MODE_RRQ 1
#define MODE_WRQ 2
#define MODE_RRQ_AND_WRQ 3

/*
//Used for netascii conversion. If desired, this may be altered when compiling for a platform other
//than Linux.
*/
#define PLATFORM_CARRIAGE_RETURN "\n"
