#pragma once

#include "connection_options.hpp"

#include "file_wrapper/file_wrapper.hpp"
#include "tftp_error.hpp"
#include "../networking/connection.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class connection_handler {
    //Constructor input associated variables
    std::shared_ptr<std::atomic<bool>> server_running;
    connection ctn;
    file_wrapper file;
    class connection_options options;
    //Internal state variables
    bool close_connection = false;
    std::vector<uint8_t> prev_packet;
    uint64_t prev_block_number = 0;
    uint32_t block_number_rollover_cnt = 0;
    //Highest level functions
    void _handle_connection();
    void respond_to_client(uint32_t timeouts = 0);
    //Packet handling functions
    void handle_packet(std::vector<uint8_t>& tftp_packet);
    void handle_RRQ_packet(std::vector<uint8_t>& tftp_packet);
    void handle_WRQ_packet(std::vector<uint8_t>& tftp_packet);
    void handle_DATA_packet(std::vector<uint8_t>& tftp_packet);
    void handle_ACK_packet(std::vector<uint8_t>& tftp_packet);
    //Packet decoding functions
    uint16_t decode_opcode(std::vector<uint8_t>& tftp_packet);
    int64_t decode_block_number(std::vector<uint8_t>& tftp_packet);
    std::unordered_map<std::string, std::string> decode_request_header(std::vector<uint8_t>& tftp_packet);
    std::unordered_map<std::string, int32_t> decode_request_header_options(std::vector<uint8_t>& tftp_packet);
    //Packet construction helper functions
    void append_network_short(std::vector<uint8_t>& data, const uint16_t number);
    void to_lowercase(std::string& str);
    //Packet construction functions
    std::vector<uint8_t> create_data_packet();
    std::vector<uint8_t> create_ack_packet();
    std::vector<uint8_t> create_oack_packet(std::unordered_map<std::string, int32_t> requested_options);
    std::vector<uint8_t> create_error_packet(tftp_error& error);
    //Packet sending functions
    void send(const std::vector<uint8_t>& data);
    void resend_packet();
    void send_error_packet(tftp_error& error);
    void terminate_if_last_packet(std::vector<uint8_t>& packet);
    //Function for opening file with proper error checking
    void open_file();
public:
    #ifdef DEBUG_BUILD
    connection_handler(std::shared_ptr<std::atomic<bool>> server_running, connection ctn,
                       std::string tftp_root, int32_t mode, bool allow_leaving_tftp_root,
                       double drop_rate, double duplicate_rate, double bitflip_rate) {
    #else
    connection_handler(std::shared_ptr<std::atomic<bool>> server_running, connection ctn,
                       std::string tftp_root, int32_t mode, bool allow_leaving_tftp_root) {
    #endif
        this->server_running = server_running;
        this->ctn = ctn;
        this->options.set("mode", mode);
        this->options.set("allow_leaving_tftp_root", allow_leaving_tftp_root);
        this->options.set("tftp_root", tftp_root);
        #ifdef DEBUG_BUILD
        this->options.set("drop_rate", drop_rate);
        this->options.set("duplicate_rate", duplicate_rate);
        this->options.set("bitflip_rate", bitflip_rate);
        #endif
    };
    #ifdef DEBUG_BUILD
    static void handle_connection(std::shared_ptr<std::atomic<bool>> server_running, connection ctn,
                                  std::string tftp_root, int32_t mode,
                                  bool allow_leaving_tftp_root, double drop, double duplicate,
                                  double bitflip);
    #else
    static void handle_connection(std::shared_ptr<std::atomic<bool>> server_running, connection ctn,
                                  std::string tftp_root, int32_t mode,
                                  bool allow_leaving_tftp_root);
    #endif
};
