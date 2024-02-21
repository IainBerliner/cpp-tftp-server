#include "tftp_server.hpp"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>

#include "../networking/networking.hpp"
#include "../handle_connection/path_resolution/path_resolution.hpp"

void* start_server_in_thread(void* thread_input_struct) {
    start_tftp_server_input_struct* param = (start_tftp_server_input_struct*)(thread_input_struct);
    #ifndef TESTING_BUILD
    tftp_listen(param->port, param->tftp_root, param->permitted_operations, param->allow_leaving_tftp_root, param->running);
    #else
    tftp_listen(param->port, param->tftp_root, param->permitted_operations, param->allow_leaving_tftp_root,
    		    param->drop, param->duplicate, param->bitflip, param->running);
    #endif
    pthread_exit(NULL);
}

bool is_port_valid (std::string input_string) {
    try {
        int port = std::stoi(input_string);
        if (port < 0) {
            return false;
        }
        else {
            return true;
        }
    }
    catch (...) {
        return false;
    }
}
bool is_tftp_root_valid (std::string input_string) {
    std::string canonical_tftp_root = resolve_path(input_string);
    if (canonical_tftp_root == "INVALID PATH" || canonical_tftp_root[canonical_tftp_root.size() - 1] != '/') {
        return false;
    }
    return true;
}
bool is_permitted_operations_valid (std::string input_string) {
    if (input_string != "RRQ" && input_string != "WRQ" && input_string != "RRQ+WRQ") {
        return false;
    }
    return true;
}
#ifdef TESTING_BUILD
bool is_drop_valid (float input_float) {
    if (input_float < 0.0 || input_float > 1.0) {
        return false;
    }
    return true;
}
bool is_duplicate_valid (float input_float) {
    if (input_float < 0.0 || input_float > 1.0) {
        return false;
    }
    return true;
}
bool is_bitflip_valid (float input_float) {
    if (input_float < 0.0 || input_float > 1.0) {
        return false;
    }
    return true;
}
#endif

std::string tftp_server::start() {
    if (!this->running()) {
        //Validate the server's parameters
        if(!(is_port_valid(this->port))) {
            return "error: failed to start because port is invalid.";
        }
        else if(!(is_tftp_root_valid(this->tftp_root))) {
            return "error: failed to start because tftp_root is invalid.";
        }
        else if(!(is_permitted_operations_valid(this->permitted_operations))) {
            return "error: failed to start because permitted_operations is invalid.";
        }
        #ifdef TESTING_BUILD
        else if(!(is_drop_valid)) {
            return "error: failed to start because packet drop percentage is invalid.";
        }
        else if(!(is_duplicate_valid)) {
            return "error: failed to start because packet duplicate percentage is invalid.";
        }
        else if(!(is_bitflip_valid)) {
            return "error: failed to start because packet bitflip percentage is invalid.";
        }
        #endif
        else {
            std::cout << "server: starting TFTP server in directory: " << "\"" << this->tftp_root << "\"" << std::endl;
        }
        //Start the thread
        this->param.port = (char*)this->port.c_str();
        this->param.tftp_root = (char*)this->tftp_root.c_str();
        this->param.permitted_operations = (char*)this->permitted_operations.c_str();
        this->param.allow_leaving_tftp_root = this->allow_leaving_tftp_root;
        #ifdef TESTING_BUILD
        this->param.drop = this->drop;
        this->param.duplicate = this->duplicate;
        this->param.bitflip = this->bitflip;
        #endif
        this->param.running = this->_running;
        if(pthread_create(&(this->thread), NULL, start_server_in_thread, (void*)&(this->param))) {
            perror("tftp_server: ERROR creating thread");
            return "error: failed to create thread.";
        }
        else {
            //Set this->_running to true
            *(bool*)this->_running.get() = true;
        }
    }
    return "success";
}

void tftp_server::stop() {
    if (this->running()) {
        //Set this->_running to false
        *(bool*)this->_running.get() = false;
        //Stop the thread
        pthread_cancel(this->thread);
        pthread_join(this->thread, NULL);
    }
    return;
}

bool tftp_server::running() {
    return *(bool*)this->_running.get();
}

pthread_t tftp_server::sockfd() {
    return this->thread;
}
