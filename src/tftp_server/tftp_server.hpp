#pragma once

#include "../networking/connection.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

class tftp_server {
    std::jthread thread;
    int32_t port;
    std::string tftp_root;
    bool enable_reading;
    bool enable_writing;
    bool allow_leaving_tftp_root;
    #ifdef DEBUG_BUILD
    float drop;
    float duplicate;
    float bitflip;
    #endif
    std::shared_ptr<std::atomic<bool>> _running = std::make_shared<std::atomic<bool>>(false);
    bool is_port_valid();
    bool is_tftp_root_valid();
    bool is_mode_valid();
    int32_t get_mode();
    #ifdef DEBUG_BUILD
    bool is_drop_valid();
    bool is_duplicate_valid();
    bool is_bitflip_valid();
    #endif
    void listen();
    void answer_new_client(connection& ctn);
public:
    #ifndef DEBUG_BUILD
    tftp_server(int32_t port, const std::string& tftp_root, bool enable_reading,
                bool enable_writing, bool allow_leaving_tftp_root): port(port),
                tftp_root(tftp_root), enable_reading(enable_reading),
                enable_writing(enable_writing), allow_leaving_tftp_root(allow_leaving_tftp_root) { }
    #else
    tftp_server(int32_t port, const std::string& tftp_root, bool enable_reading,
                bool enable_writing, bool allow_leaving_tftp_root, float drop, float duplicate,
                float bitflip): port(port), tftp_root(tftp_root), enable_reading(enable_reading),
                enable_writing(enable_writing), allow_leaving_tftp_root(allow_leaving_tftp_root),
                drop(drop), duplicate(duplicate), bitflip(bitflip) { }
    #endif
    ~tftp_server() {
        this->stop();
    }
    std::string start();
    void stop();
    bool running();
    void join();
};
