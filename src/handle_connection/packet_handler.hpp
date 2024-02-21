#include "connection_options.hpp"

#include <string>
#include <vector>

#include "../macros/packet_handling_macros.hpp"

class packet_handler {
    int client_sockfd;
    struct connection_options options;
    bool _should_exit = false;
    bool sent_last_packet = false;
    std::vector<unsigned char> last_sent_packet = {};
    //The below variable is used to ensure packets are in order.
       long last_received_block_number = 0;
       //if the transfer size exceeds 65536 blocks, the below variable helps the code to roll over and continue from 0
       int block_number_offset = 0;
    public:
    packet_handler(int input_client_sockfd, std::string input_tftp_root, std::string input_permitted_operations, bool allow_leaving_tftp_root): client_sockfd(input_client_sockfd) {
        this->options = get_default_options();
        this->options.permitted_operations = input_permitted_operations;
        this->options.allow_leaving_tftp_root = allow_leaving_tftp_root;
        this->options.tftp_root = input_tftp_root;
        this->resolve_tftp_root();
    };
    void resolve_tftp_root();
    bool should_exit();
    void retransmit_last_packet();
    int timeout();
    void handle_packet(std::vector<unsigned char> tftp_packet);
};
