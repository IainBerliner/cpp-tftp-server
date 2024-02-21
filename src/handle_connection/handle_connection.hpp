#ifndef PACKET_HANDLING_HPP
#define PACKET_HANDLING_HPP
#include <atomic>
#include <memory>

#include "../macros/constants.hpp"

//intended for use with the tftp_listen() function from networking.hpp
void* handle_connection(void* param);
struct handle_connection_input_struct {
    //information for the send call
    int sockfd;
    //initial packet
    char buf[MAXBUFLEN];
    int numbytes;
    //tftp server root and the operations that the server permits (RRQ/WRQ/BOTH)
    const char* tftp_root;
    const char* permitted_operations;
    bool allow_leaving_tftp_root;
    #ifdef TESTING_BUILD
    float drop;
    float duplicate;
    float bitflip;
    #endif
    //flag so child threads can shut down when parent is no longer running
    std::shared_ptr<std::atomic<bool>> parent_thread_alive;
};
#endif
