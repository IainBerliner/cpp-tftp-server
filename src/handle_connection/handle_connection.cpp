#include "handle_connection.hpp"

#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

//#include <atomic>
#include <climits>
#include <cstdio>
#include <iostream>
#include <map>
//#include <memory>
#include <string>
#include <vector>

#include "packet_handler.hpp"
#include "../macros/pthread_macros.hpp"
#include "../networking/networking.hpp"

#ifdef TESTING_BUILD
#include "../random_functions/random_functions.hpp"
#endif

#if CHAR_BIT != 8
#error Building this repository on systems where bytelength > 8 bits is not supported.
#endif

void close_sockfd_wrapper(void* input_pointer) {
    close(*(int*)(input_pointer));
}

void* handle_connection(void* param) {
    
    THREAD_START()
    
    //Detach itself
    pthread_detach(pthread_self());
    int numbytes;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    
    struct handle_connection_input_struct* thread_input = (handle_connection_input_struct*)param;
    
    #ifdef TESTING_BUILD
    static float drop_chance = thread_input->drop;
    static float duplicate_chance = thread_input->duplicate;
    static float bitflip_chance = thread_input->bitflip;
    #endif
    
    THREAD_REGISTER_CLEANUP(CLOSE_SOCKFD, thread_input->sockfd)
    THREAD_REGISTER_CLEANUP(DELETE_HANDLE_CONNECTION_INPUT_STRUCT, param);
    
    //for incoming packets
    std::vector<unsigned char> incoming_packet = {};
    //For handling incoming packets
    packet_handler connection_packet_handler = packet_handler(thread_input->sockfd, std::string(thread_input->tftp_root),
                                      std::string(thread_input->permitted_operations), thread_input->allow_leaving_tftp_root);
    //Deal with first packet
    for (int i = 0; i < thread_input->numbytes; i++) {
        incoming_packet.push_back(thread_input->buf[i]);
    }
    #ifdef TESTING_BUILD
    printf("server: got new packet.\n");
    printf("server: packet is %d bytes long\n", thread_input->numbytes);
    #endif
    #ifndef TESTING_BUILD
    connection_packet_handler.handle_packet(incoming_packet);
    #endif
    #ifdef TESTING_BUILD
    flip_random_bits(incoming_packet, bitflip_chance);
    if (!get_random_bool(drop_chance)) {
        if (get_random_bool(duplicate_chance)) {
            connection_packet_handler.handle_packet(incoming_packet);
            connection_packet_handler.handle_packet(incoming_packet);
        }
        else {
            connection_packet_handler.handle_packet(incoming_packet);
        }
    }
    #endif
    
    while (!connection_packet_handler.should_exit()) {
        if (!(*(thread_input->parent_thread_alive.get()))) { pthread_exit(NULL); }
        THREAD_SPECIFY_CANCELLATION_POINT(numbytes = recvtimeout(thread_input->sockfd, buf, MAXBUFLEN-1, 0, connection_packet_handler.timeout(), 0))
        if (numbytes == -1) {
            perror("recv");
            break;
        }
        else if (numbytes == -2) {
            //retransmit last packet and try again if timeout happens
            #ifdef TESTING_BUILD
            std::cout << "server: connection timed out. Resending last packet." << std::endl;
            #endif
            connection_packet_handler.retransmit_last_packet();
            //Start another timeout period
            if (!(*(thread_input->parent_thread_alive.get()))) { pthread_exit(NULL); }
            THREAD_SPECIFY_CANCELLATION_POINT(numbytes = recvtimeout(thread_input->sockfd, buf, MAXBUFLEN-1, 0, connection_packet_handler.timeout(), 0))
            if (numbytes == -1) {
                perror("recv");
                break;
            }
            else if (numbytes == -2) {
                #ifdef TESTING_BUILD
                std::cout << "server: connection timed out again. Forgetting connection." << std::endl;
                #endif
                break;
            }
        }
        #ifdef TESTING_BUILD
        printf("server: got new packet.\n");
        printf("server: packet is %d bytes long\n", thread_input->numbytes);
        #endif
        incoming_packet = {};
        for (int i = 0; i < numbytes; i++) {
            incoming_packet.push_back(buf[i]);
        }
        #ifndef TESTING_BUILD
        connection_packet_handler.handle_packet(incoming_packet);
        #endif
        #ifdef TESTING_BUILD
        flip_random_bits(incoming_packet, bitflip_chance);
        if (!get_random_bool(drop_chance)) {
            if (get_random_bool(duplicate_chance)) {
                connection_packet_handler.handle_packet(incoming_packet);
                connection_packet_handler.handle_packet(incoming_packet);
            }
            else {
                connection_packet_handler.handle_packet(incoming_packet);
            }
        }
        #endif
    }
    
    #ifdef TESTING_BUILD
    std::cout << "server: closing connection." << std::endl;
    #endif
    
    //Cleanup everything
    THREAD_CLEANUP()
    
    //Exit
    pthread_exit(NULL);
}
