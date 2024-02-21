#ifndef TFTP_SERVER_HPP
#define TFTP_SERVER_HPP

#include <pthread.h>

#include <atomic>
#include <memory>
#include <string>

struct start_tftp_server_input_struct {
    char* port;
    char* tftp_root;
    char* permitted_operations;
    bool allow_leaving_tftp_root;
    #ifdef TESTING_BUILD
    float drop;
    float duplicate;
    float bitflip;
    #endif
    std::shared_ptr<std::atomic<bool>> running;
};

class tftp_server {
    pthread_t thread;
    start_tftp_server_input_struct param;
    std::string port;
    std::string tftp_root;
    std::string permitted_operations;
    bool allow_leaving_tftp_root;
    #ifdef TESTING_BUILD
    float drop;
    float duplicate;
    float bitflip;
    #endif
    std::shared_ptr<std::atomic<bool>> _running = std::make_shared<std::atomic<bool>>(false);
    public:
    #ifndef TESTING_BUILD
    tftp_server(const std::string input_port, const std::string input_tftp_root, const std::string input_permitted_operations, const bool input_allow_leaving_tftp_root):
    port(input_port), tftp_root(input_tftp_root), permitted_operations(input_permitted_operations), allow_leaving_tftp_root(input_allow_leaving_tftp_root) {
    #else
    tftp_server(const std::string input_port, const std::string input_tftp_root, const std::string input_permitted_operations, const bool input_allow_leaving_tftp_root,
    			float input_drop, float input_duplicate, float input_bitflip):
    port(input_port), tftp_root(input_tftp_root), permitted_operations(input_permitted_operations), allow_leaving_tftp_root(input_allow_leaving_tftp_root),
    drop(input_drop), duplicate(input_duplicate), bitflip(input_bitflip) {
    #endif
        this->start();        
    }
    ~tftp_server() {
        this->stop();
    }
    std::string start();
    void stop();
    bool running();
    pthread_t sockfd();
};
#endif
