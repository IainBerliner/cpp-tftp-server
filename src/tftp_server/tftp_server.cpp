#include "tftp_server.hpp"

#include "../connection_handler/connection_handler.hpp"
#include "../networking/connection.hpp"
#include "../networking/networking.hpp"
#include "../macros/constants.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

bool tftp_server::is_port_valid () {
    return (port >= 0 && port <= 65535);
}

bool tftp_server::is_tftp_root_valid() {
    if (std::filesystem::exists(this->tftp_root)) {
        this->tftp_root = std::filesystem::weakly_canonical(this->tftp_root);
        return true;
    }
    return false;
}

bool tftp_server::is_mode_valid() {
    return (enable_reading || enable_writing);
}

int32_t tftp_server::get_mode() {
    if (enable_reading && enable_writing) { return MODE_RRQ_AND_WRQ; }
    else if (enable_reading) { return MODE_RRQ; }
    else if (enable_writing) { return MODE_WRQ; }
    else { return 0; }
}

#ifdef DEBUG_BUILD
bool tftp_server::is_drop_valid () {
    if (drop < 0.0 || drop > 1.0) {
        return false;
    }
    return true;
}

bool tftp_server::is_duplicate_valid () {
    if (duplicate < 0.0 || duplicate > 1.0) {
        return false;
    }
    return true;
}

bool tftp_server::is_bitflip_valid () {
    if (bitflip < 0.0 || bitflip > 1.0) {
        return false;
    }
    return true;
}
#endif

std::string tftp_server::start() {
    //Validate the server's parameters
    if (this->running()) {
        return "error: server already running.";
    }
    else if(!(this->is_port_valid())) {
        return "error: failed to start because port is invalid.";
    }
    else if(!(this->is_tftp_root_valid())) {
        return "error: failed to start because tftp_root is invalid.";
    }
    else if(!(this->is_mode_valid())) {
        return "error: failed to start because neither reading nor writing has been enabled.";
    }
    #ifdef DEBUG_BUILD
    else if(!(this->is_drop_valid())) {
        return "error: failed to start because packet drop percentage is invalid.";
    }
    else if(!(this->is_duplicate_valid())) {
        return "error: failed to start because packet duplicate percentage is invalid.";
    }
    else if(!(this->is_bitflip_valid())) {
        return "error: failed to start because packet bitflip percentage is invalid.";
    }
    #endif
    else {
        std::cout << "tftp_server::start: starting TFTP server." << std::endl;
        std::cout << "Directory: " << "\"" << this->tftp_root << "\"" << std::endl;
        std::cout << "Port: " << this->port << std::endl;
    }
    //Start the thread
    thread = std::jthread(std::bind(&tftp_server::listen, this));
    if(!thread.joinable()) {
        std::cerr << "tftp_server::start: error creating thread." << std::endl;
        return "error: failed to create thread.";
    }
    else {
        //Set this->_running to true
        *(bool*)this->_running.get() = true;
    }
    return "success";
}

void tftp_server::listen() {
    while (1) {
        connection ctn(port);
        if (!ctn.valid()) { std::cout << "tftp_server::listen: connection invalid." << std::endl; this->stop(); }
        while (ctn.listen(1.0, 0.0) == -2) { if (!this->running()) return; }
        this->answer_new_client(ctn);
    }
    
    return;
}

void tftp_server::answer_new_client(connection& ctn) {
    #ifdef DEBUG_BUILD
    auto new_thread = std::jthread(&connection_handler::handle_connection, _running, ctn, tftp_root,
                                   this->get_mode(), allow_leaving_tftp_root, drop,
                                   duplicate, bitflip);
    #else
    auto new_thread = std::jthread(&connection_handler::handle_connection, _running, ctn, tftp_root,
                                   this->get_mode(), allow_leaving_tftp_root);
    #endif
    if (!new_thread.joinable()) {
        std::cerr << "tftp_server::answer_new_client: error creating thread." << std::endl;
        this->stop();
    }
    else { new_thread.detach(); }
}

void tftp_server::stop() {
    if (this->running()) {
        std::cout << "tftp_server::stop: stopping TFTP server." << std::endl;
        //Set this->_running to false
        *(bool*)this->_running.get() = false;
    }
    return;
}

bool tftp_server::running() {
    return *(bool*)this->_running.get();
}

void tftp_server::join() {
  if (thread.joinable()) {
    this->thread.join();
  }
  return;
}
