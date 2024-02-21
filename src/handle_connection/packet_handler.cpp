#include "packet_handler.hpp"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <climits>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "connection_options.hpp"
#include "netascii.hpp"
#include "path_resolution/path_resolution.hpp"
#include "../argument_handling/handle_arguments.hpp"
#include "../macros/constants.hpp"
#include "../macros/packet_handling_macros.hpp"
#include "../networking/networking.hpp"

const std::map<unsigned int, std::string> TFTP_OPCODES = {{1, std::string("RRQ")},
                         {2, std::string("WRQ")},
                         {3, std::string("DATA")},
                         {4, std::string("ACK")}, 
                         {5, std::string("ERROR")},
                         {6, std::string("OACK")}};

const char* TRANSFER_MODES[] = {"netascii", "octet", "mail"};

const std::map<unsigned int, std::string> TFTP_ERRORS = {{0, std::string("Not Defined")},
                        {1, std::string("File Not Found")},
                        {2, std::string("Access Violation")},
                        {3, std::string("Disk Full or Allocation Exceeded")},
                        {4, std::string("Illegal TFTP Operation")}, 
                        {5, std::string("Unknown Transfer TID")},
                        {6, std::string("File Already Exists")},
                        {7, std::string("No Such User")},
                        {8, std::string("Option Request Denied")}};

//Network byte order!
std::vector<unsigned char> uint_to_network_short(const unsigned int block_number) {
    const unsigned short network_short = htons((unsigned short)(block_number));
    const char* network_short_as_char_array = (const char*)&network_short;
    std::vector<unsigned char> result = {};
    result.push_back((unsigned char)(network_short_as_char_array[0]));
    result.push_back((unsigned char)(network_short_as_char_array[1]));
    return result;
}

std::vector<unsigned char> read_file(unsigned int block_number, std::string filepath, std::string mode, unsigned short data_size, bool* read_fail);

std::vector<unsigned char> rollover_data(const std::vector<unsigned char> input_data, const unsigned short data_size) {
    std::vector<unsigned char> output_data = {};
    //Remember extra data per-connection
    thread_local static std::vector<unsigned char> stored_data = {};
    //read input data into stored data
    for (int i = 0; i < input_data.size(); i++) {
        stored_data.push_back(input_data[i]);
    }
    //read stored data into output data
    for (int i = 0; i < data_size && stored_data.size() > 0; i++) {
        output_data.push_back(stored_data[0]);
        stored_data.erase(stored_data.begin());
    }
    //return output data
    return output_data;
}

//Note: this function includes a bunch of esoteric-seeming extra stuff to ensure that packets are the proper size,
//because read_file may not always return the data_size requested.
//This has to do with Netascii formatting. Including the extra code in this function to deal with that seemed like a reasonable solution to me,
//rather than creating temp files that get converted to/from netascii.
std::vector<unsigned char> create_data_packet(const long block_number, const std::string filename, const std::string mode, const unsigned short data_size) {
    //Remember by how far we have read ahead in the file per-connection
    thread_local static int extra_block_offset = 0;
    //handle block number rollover
    const unsigned short block_number_truncated = block_number % MAX_U_NETWORK_SHORT_SIZE;
    #ifdef TESTING_BUILD
    std::cout << "server: sending block number: " << block_number_truncated << std::endl;
    #endif
    std::vector<unsigned char> data = {};
    //Append data op code (03)
    data.push_back(0);
    data.push_back(3);
    //Append block number
    const std::vector<unsigned char> block_number_as_network_short = uint_to_network_short(block_number_truncated);
    data.insert(data.end(), block_number_as_network_short.begin(), block_number_as_network_short.end());
    //Append content
    //NOTE: how to check if filepath is valid?
    bool read_fail[1] = {false};
    //read out stored data first
    std::vector<unsigned char> content = {};
    content = rollover_data(content, data_size);
    //If the content is less than it should be and it is not the end of the file, then read ahead by one extra block,
    //and append it to previous content. Keep doing this until it is the minimum size or we run out of blocks in the file
    extra_block_offset--;
    while (content.size() < data_size && !(*read_fail)) {
        extra_block_offset++;
        std::vector<unsigned char> new_content = read_file(block_number + extra_block_offset, filename, mode, data_size, read_fail);
        content.insert(content.end(), new_content.begin(), new_content.end());
    }
    //Now make sure that the content does not go over the minimum size, by invoking a function that stores the extra data and then doles it out
    //to subsequent calls to this function
    content = rollover_data(content, data_size);
    #ifdef TESTING_BUILD
    std::cout << "create_data_packet: the content size: " << content.size() << std::endl;
    std::cout << "create_data_packet: whether the read failed: " << *read_fail << std::endl;
    #endif
    //Finally just insert the data into the packet
    data.insert(data.end(), content.begin(), content.end());
    return data;
}

std::vector<unsigned char> create_ack_packet(const long block_number) {
    //Handle block number rollover
    const unsigned short block_number_truncated = block_number % MAX_U_NETWORK_SHORT_SIZE;
    std::vector<unsigned char> ack = {};
    //Append acknowledgement opcode (04)
    ack.push_back(0);
    ack.push_back(4);
    //Append block number
    const std::vector<unsigned char> block_number_as_network_short = uint_to_network_short(block_number_truncated);
    ack.insert(ack.end(), block_number_as_network_short.begin() ,block_number_as_network_short.end());
    return ack;
}

std::vector<unsigned char> create_oack_packet(std::map<std::string, int> requested_options) {
    std::vector<unsigned char> oack = {};
    //Append option acknowledgement opcode (06)
    oack.push_back(0);
    oack.push_back(6);
    //append option string/value pairs for supported options
    for (auto i: requested_options) {
        const char* option_as_c_str = i.first.c_str();
        for (int j = 0; j < i.first.size() + 1; j++) {
            oack.push_back(option_as_c_str[j]);
        }
        std::string value_as_string = std::to_string(i.second);
        const char* value_as_c_str = value_as_string.c_str();
        for (int j = 0; j < value_as_string.size() + 1; j++) {
            oack.push_back(value_as_c_str[j]);
        }
    }
    return oack;
}

std::vector<unsigned char> create_error_packet(const unsigned int error_code) {
    std::vector<unsigned char> err = {};
    err.push_back(0);
    err.push_back(5);
    //Append error code
    const std::vector<unsigned char> error_code_as_network_short = uint_to_network_short(error_code);
    err.insert(err.end(), error_code_as_network_short.begin(), error_code_as_network_short.end());
    //Append error message followed by null terminator
    const std::string error_message = TFTP_ERRORS.at(error_code);
    const char* error_message_as_c_str = error_message.c_str();
    for (int i = 0; i < error_message.size() + 1; i++) {
        err.push_back(error_message_as_c_str[i]);
    }
    return err;
}

void send_packet(std::vector<unsigned char> packet, int client_sockfd) {
    send(client_sockfd, (const void*)packet.data(), packet.size(), 0);
    return;
}

/*
/* Get data from file either in blocks of 512 bits or whatever is left
/* Note: since this function returns empty data if the file does not exist,
/* functions that use this function should ensure that they do,
/* in order to prevent requests for erroneous files being returned as empty files from the server
/* (rather than raising an error for the file not existing)
*/
std::vector<unsigned char> read_file(unsigned int block_number, std::string filepath, std::string mode, unsigned short data_size, bool* read_fail) {
    const unsigned int offset = (block_number - 1) * data_size;
    std::ifstream file_stream;
    file_stream.open(filepath, std::ios_base::binary);
    file_stream.seekg(offset);
    std::vector<unsigned char> result(data_size);
    file_stream.read((char*)result.data(), data_size);
    //If we hit EOF we need to use special logic to properly fill output data vector
    if (file_stream.eof()) {
        file_stream.close();
        result.clear();
        
        file_stream.open(filepath);
        file_stream.seekg(offset);
        
        while(file_stream.peek() != EOF) {
            char next_byte[1];
            file_stream.read(next_byte, 1);
            result.push_back(*next_byte);
        }
        *read_fail = true;
    }
    else if (!file_stream.good()){
        //If there is an issue unrelated to EOF (maybe the file doesn't exist or something) return empty data.
        result.clear();
        *read_fail = true;
    }
    else {
        *read_fail = false;
    }
    file_stream.close();
    //handle netascii data
    if (mode == "netascii") {
        result = to_net_ascii(result, 0, result.size());
    }
    return result;
}

void write_file(const std::vector<unsigned char> new_data, long block_number, std::string filepath, std::string mode) {
    std::ofstream file_stream;
    char* data_pointer;
    int data_size;
    if (mode == "netascii") {
        const std::vector<unsigned char> host_ascii = from_net_ascii(new_data, 4, new_data.size());
        data_pointer = (char*)(host_ascii.data());
        data_size = host_ascii.size();
    }
    else {
        data_pointer = (char*)new_data.data() + 4;
        data_size = new_data.size() - 4;
    }
    if (block_number == 1) {
        file_stream.open(filepath, std::ios_base::binary | std::ios_base::trunc);
    }
    else {
        file_stream.open(filepath, std::ios_base::binary | std::ios_base::app);
    }
    if (file_stream.fail()) {
        std::cout << strerror(errno) << std::endl;
    }
    #ifdef TESTING_BUILD
    std::cout << "write_file: data size is: " << data_size << std::endl;
    std::cout << "write_file: the data is: ";
    #endif
    for (int i = 0; i < data_size; i++) {
        std::cout << data_pointer[i];
    }
    std::cout << std::endl;
    file_stream.write(data_pointer, data_size);
    if (file_stream.good()) {
        std::cout << "File write successful." << std::endl;
        file_stream.close();
        return;
    }
    std::cout << "File write error." << std::endl;
    file_stream.close();
    return;
}


//Get opcode from TFTP header
std::string get_opcode(std::vector<unsigned char> tftp_packet) {
    if (tftp_packet.size() < 2) {
        throw 4;
    }
    const unsigned short opcode = ntohs(*((unsigned short*)(&tftp_packet[0])));
    if (TFTP_OPCODES.find(opcode) != TFTP_OPCODES.end()) {
        return TFTP_OPCODES.at(opcode);
    }
    else {
        throw 0;
    }
}

//Return filename and mode from decoded RRQ/WRQ header
std::vector<std::string> decode_request_header(std::vector<unsigned char> tftp_packet) {
    std::vector<int> zero_locations = {};
    
    for (int i = 2; i < tftp_packet.size(); i++) {
        if(tftp_packet[i] == (unsigned char)0) {
            zero_locations.push_back(i);
        }
    }
    
    if (zero_locations.size() < 2) {
        //if there aren't null termined strings for the filename and mode, throw an error.
        throw 4;
    }
    
    std::string file_name = (char *)(tftp_packet.data() + 2);
    std::string mode = (char *)(tftp_packet.data() + zero_locations[0] + 1);
    
    //Convert the mode parameter to lower case
    std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c){ return std::tolower(c); } );
    
    return {file_name, mode};
}

std::map<std::string, int> decode_request_header_options(std::vector<unsigned char> tftp_packet) {
    std::vector<int> zero_locations = {};
    std::map<std::string, int> options_map = {};
    
    for (int i = 2; i < tftp_packet.size(); i++) {
        if(tftp_packet[i] == (unsigned char)0) {
            zero_locations.push_back(i);
        }
    }
    
    if (zero_locations.size() % 2 != 0) {
        //options (which are null terminated strings) should always come in pairs with their values (which are null terminated strings)
        //therefore, throw an exception if this is not the case.
        throw 4;
    }
    
    for (int i = 2; i < zero_locations.size(); i=i+2) {
        std::string option = (char*)&tftp_packet[zero_locations[i - 1] + 1];
        
        //Convert the option parameter to lower case
        std::transform(option.begin(), option.end(), option.begin(), [](unsigned char c){ return std::tolower(c); } );
        
        std::string value = (char*)&tftp_packet[zero_locations[i] + 1];
        options_map[option] = std::stoi(value);
    }
    
    for (auto i: options_map) {
    	#ifdef TESTING_BUILD
        std::cout << "decode_request_header_options: option: " << i.first << std::endl;
        std::cout << "decode_request_header_options: value: " << i.second << std::endl;
        #endif
    }
    
    return options_map;
}

//Return the block number from a decoded ACK packet
unsigned short decode_block_number(std::vector<unsigned char> tftp_packet) {
    
    if (tftp_packet.size() < 4) {
        //If there is no block number to decode, throw an exception.
        throw 4;
    }
    
    /*
    unsigned short ushort_pointer[1];
    memcpy(ushort_pointer, &tftp_packet[2], sizeof(unsigned short));
    unsigned short block_number = *ushort_pointer;*/
    
    const unsigned short block_number = ntohs(*((unsigned short*)(&tftp_packet[2])));
    return block_number;
}

bool file_exists (const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

void packet_handler::resolve_tftp_root() {
    #ifdef TESTING_BUILD
    std::cout << "packet_handler::resolve_tftp_root: the tftp root: " << this->options.tftp_root << std::endl;
    #endif
    this->options.tftp_root = resolve_path(this->options.tftp_root);
    #ifdef TESTING_BUILD
    std::cout << "packet_handler::resolve_tftp_root: the canonical path of tftp root: " << this->options.tftp_root << std::endl;
    #endif
    return;
}

bool packet_handler::should_exit() {
    return this->_should_exit;
}

void packet_handler::retransmit_last_packet() {
    send_packet(this->last_sent_packet, this->client_sockfd);
    return;
}

int packet_handler::timeout() {
    return this->options.timeout;
}

//Handle incoming packets: this function can contain nearly all of the server-side logic,
//because TFTP is almost stateless.
void packet_handler::handle_packet(std::vector<unsigned char> tftp_packet) {
    
    std::string opcode;
    CATCH_INPUT_VALIDATION_ERRORS(opcode = get_opcode(tftp_packet))
    
    #ifdef TESTING_BUILD
    std::cout << "packet_handler::handle_packet: op code: " << opcode << std::endl;
    #endif
    
    if (opcode == "RRQ") {
        #ifdef TESTING_BUILD
        std::cout << "server: got an RRQ packet." << std::endl;
        #endif
        
        //Check that reading mode is enabled. If it is not, send an error packet and close the connection.
        if (!(options.permitted_operations == "RRQ" || options.permitted_operations == "RRQ+WRQ")) {
            //If the reading mode is not enabled, send an error packet.
            const std::vector<unsigned char> error_packet = create_error_packet(2);
            send_packet(error_packet, this->client_sockfd);
            this->last_sent_packet = error_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending error packet to client because reading mode is not enabled." << std::endl;
            #endif
            this->_should_exit = true;
            return;
        }
        
        std::vector<std::string> file_name_and_mode;
        CATCH_INPUT_VALIDATION_ERRORS(file_name_and_mode = decode_request_header(tftp_packet))
        
        //Send error packet if client's relative path leaves tftp-root directory, even temporarily, if connection_options allow_leaving_tftp_root isn't set to true
        if (!options.allow_leaving_tftp_root && path_leaves_directory(options.tftp_root, std::string(file_name_and_mode[0]))) {
            const std::vector<unsigned char> error_packet = create_error_packet(2);
            send_packet(error_packet, this->client_sockfd);
            this->last_sent_packet = error_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending error packet because the path passed by client leaves tftp-root at some point along its resolution." << std::endl;
            #endif
            this->_should_exit = true;
            return;
        }
        
        this->options.filename = options.tftp_root + std::string(file_name_and_mode[0]);
        this->options.transfer_mode = std::string(file_name_and_mode[1]);
        #ifdef TESTING_BUILD
        std::cout << "packet_handler::handle_packet: file name: " << this->options.filename << std::endl;
        std::cout << "packet_handler::handle_packet: transfer mode: " << this->options.transfer_mode << std::endl;
        #endif
        
        this->options.operation = std::string("RRQ");
        
        //Check that file exists. If it doesn't, send an error packet and close the connection.
        if (!file_exists(options.filename)) {
            //If the file already exists, send an error packet and close the connection.
            const std::vector<unsigned char> error_packet = create_error_packet(1);
            send_packet(error_packet, this->client_sockfd);
            this->last_sent_packet = error_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending error packet to client because the file the client requested to read does not exist." << std::endl;
            #endif
            this->_should_exit = true;
            return;
        }
        
        //Get options from request header
        std::map<std::string, int> requested_options;
        CATCH_INPUT_VALIDATION_ERRORS(requested_options = decode_request_header_options(tftp_packet))
        CATCH_INPUT_VALIDATION_ERRORS(sanitize_options(requested_options, this->options))
        
        if (requested_options.size() == 0) {
            //Send data packet to acknowledge RRQ
            const std::vector<unsigned char> data_packet = create_data_packet(1, this->options.filename, this->options.transfer_mode, this->options.data_size);
            send_packet(data_packet, this->client_sockfd);
            this->last_sent_packet = data_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending data packet to client to acknowledge RRQ." << std::endl;
            #endif
            
            //Handle last packet of data (if it happens to be the first).
            if ((data_packet.size() - 4) < this->options.data_size) {
                    this->sent_last_packet = true;
            }
            
            return;
        }
        else {
            //Send oack packet
            const std::vector<unsigned char> oack_packet = create_oack_packet(requested_options);
            send_packet(oack_packet, this->client_sockfd);
            this->last_sent_packet = oack_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending oack packet to client to acknowledge RRQ." << std::endl;
            #endif
            //modify options
            this->options = modify_options(this->options, requested_options);
            return;
        }
    }
    else if (opcode == "WRQ") {
        #ifdef TESTING_BUILD
        std::cout << "server: got an WRQ packet." << std::endl;
        #endif
        
        //Check that writing mode is enabled. If it is not, send an error packet and close the connection.
        if (!(options.permitted_operations == "WRQ" || options.permitted_operations == "RRQ+WRQ")) {
            //If writing mode is not enabled, send an error packet.
            const std::vector<unsigned char> error_packet = create_error_packet(2);
            send_packet(error_packet, this->client_sockfd);
            this->last_sent_packet = error_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending error packet to client because writing mode is not enabled." << std::endl;
            #endif
            this->_should_exit = true;
            return;
        }
        
        std::vector<std::string> file_name_and_mode;
        CATCH_INPUT_VALIDATION_ERRORS(file_name_and_mode = decode_request_header(tftp_packet))
        //Send error packet if client's relative path leaves tftp-root directory, even temporarily, if connection_options allow_leaving_tftp_root isn't set to true
        if (!options.allow_leaving_tftp_root && path_leaves_directory(options.tftp_root, std::string(file_name_and_mode[0]))) {
            const std::vector<unsigned char> error_packet = create_error_packet(2);
            send_packet(error_packet, this->client_sockfd);
            this->last_sent_packet = error_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending error packet to client because the path passed by client leaves tftp-root at some point along its resolution." << std::endl;
            #endif
            this->_should_exit = true;
            return;
        }
        this->options.filename = options.tftp_root + std::string(file_name_and_mode[0]);
        this->options.transfer_mode = std::string(file_name_and_mode[1]);
        #ifdef TESTING_BUILD
        std::cout << "packet_handler::handle_packet: file name: " << this->options.filename << std::endl;
        std::cout << "packet_handler::handle_packet: transfer mode: " << this->options.transfer_mode << std::endl;
        #endif
        
        this->options.operation = std::string("WRQ");
        
        //RFC recommends that you don't implement mail mode.
        if (file_name_and_mode[1] == "mail") {
            const std::vector<unsigned char> error_packet = create_error_packet(0);
            send_packet(error_packet, this->client_sockfd);
            this->last_sent_packet = error_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending error packet to client because mail mode is not implemented." << std::endl;
            #endif
            this->_should_exit = true;
            return;
        }
        else {
            //Check if file already exists
            if (file_exists(options.filename)) {
                //If the file already exists, send an error packet and close the connection.
                const std::vector<unsigned char> error_packet = create_error_packet(6);
                send_packet(error_packet, this->client_sockfd);
                this->last_sent_packet = error_packet;
                #ifdef TESTING_BUILD
                std::cout << "server: sending error packet to client because the file the client requested to write to already exists." << std::endl;
                #endif
                this->_should_exit = true;
                return;
            }
            //Get options from request header
            std::map<std::string, int> requested_options;
            CATCH_INPUT_VALIDATION_ERRORS(requested_options = decode_request_header_options(tftp_packet))
            if (requested_options.size() == 0) {
                //Send ack packet
                const std::vector<unsigned char> ack_packet = create_ack_packet(0);
                send_packet(ack_packet, this->client_sockfd);
                this->last_sent_packet = ack_packet;
                #ifdef TESTING_BUILD
                std::cout << "server: sending ack packet to client to acknowledge WRQ." << std::endl;
                #endif
                return;
            }
            else {
                //Send oack packet
                const std::vector<unsigned char> oack_packet = create_oack_packet(requested_options);
                send_packet(oack_packet, this->client_sockfd);
                this->last_sent_packet = oack_packet;
                #ifdef TESTING_BUILD
                std::cout << "server: sending oack packet to client to acknowledge WRQ." << std::endl;
                #endif
                //modify options
                this->options = modify_options(this->options, requested_options);
                return;
            }
        }
    }
    else if (options.operation == "none") {
        //Send error packet if received packet is not RRQ or WRQ but the server has not already received either of these
        const std::vector<unsigned char> error_packet = create_error_packet(5);
        send_packet(error_packet, this->client_sockfd);
        this->last_sent_packet = error_packet;
        #ifdef TESTING_BUILD
        std::cout << "server: sending error packet to client because the client sent data/ack packet but server has not received RRQ/WRQ packet." << std::endl;
        #endif
        this->_should_exit = true;
        return;
    }
    //Complementary to WRQ
    else if (opcode == "DATA") {
        
        long block_number; 
        CATCH_INPUT_VALIDATION_ERRORS(block_number = decode_block_number(tftp_packet))
        //handle block number rollover
        block_number = this->block_number_offset * MAX_U_NETWORK_SHORT_SIZE + block_number;
        if (block_number == 0 && this->last_received_block_number != 0 ) {
            this->block_number_offset++;
        }
        block_number = this->block_number_offset * MAX_U_NETWORK_SHORT_SIZE + block_number;
        #ifdef TESTING_BUILD
        std::cout << "server: block number: " << block_number << std::endl;
        std::cout << "server: block number offset: " << this->block_number_offset << std::endl;
        std::cout << "server: last received block number: " << this->last_received_block_number << std::endl;
        #endif
        if (block_number == this->last_received_block_number + 1) {
            //increment last_received_block_number
            this->last_received_block_number = block_number;
            //write data
            CATCH_INPUT_VALIDATION_ERRORS(write_file(tftp_packet, block_number, this->options.filename, this->options.transfer_mode))
            const std::vector<unsigned char> ack_packet = create_ack_packet(block_number);
            send_packet(ack_packet, this->client_sockfd);
            this->last_sent_packet = ack_packet;
            std::cout << "Sending ack packet to acknowledge having received data." << std::endl;
            //terminate connection if it's the last packet
            if ((tftp_packet.size() - 4) < this->options.data_size) {
                this->_should_exit = true;
            }
            return;
        }
        else {
            //Ignore packets that are not in order.
            #ifdef TESTING_BUILD
            std::cout << "server: ignoring duplicate packet." << std::endl;
            #endif
            return;
        }
    }
    //Complementary to RRQ
    else if (opcode == "ACK") {
        
        //Gracefully ignore last ACK.
        if (this->sent_last_packet) {
            this->_should_exit = true;
            return;
        }        
        
        long block_number; 
        CATCH_INPUT_VALIDATION_ERRORS(block_number = decode_block_number(tftp_packet))
        //handle block number rollover
        if (block_number == 0 && this->last_received_block_number != 0 ) {
            this->block_number_offset++;
        }
        block_number = this->block_number_offset * MAX_U_NETWORK_SHORT_SIZE + block_number;
        #ifdef TESTING_BUILD
        std::cout << "server: block number: " << block_number << std::endl;
        std::cout << "server: block number offset: " << this->block_number_offset << std::endl;
        std::cout << "server: last received block number: " << this->last_received_block_number << std::endl;
        #endif
        
        //Important: TFTP clients may send an ACK packet with a block number of 0 in response to an OACK packet
        //That's why the second check is necessary
        if (block_number == this->last_received_block_number + 1 || block_number == 0) {
            //increment last_received_block_number
            this->last_received_block_number = block_number;
            //read data
            const std::vector<unsigned char> data_packet = create_data_packet(block_number + 1, this->options.filename, this->options.transfer_mode, this->options.data_size);
            send_packet(data_packet, client_sockfd);
            this->last_sent_packet = data_packet;
            #ifdef TESTING_BUILD
            std::cout << "server: sending data packet to client in response to ACK packet." << std::endl;
            #endif

            //Handle last packet of data.
            if ((data_packet.size() - 4) < this->options.data_size) {
                    this->sent_last_packet = true;
            }    
        
            return;
        }
        else {
            //Ignore packets that are not in order so that the "sorcerer's apprentice" bug can be avoided (also simplifies implementation).
            #ifdef TESTING_BUILD
            std::cout << "server: ignoring duplicate packet." << std::endl;
            #endif
            return;
        }
    }
    else if (opcode == "ERROR") {
        //Terminate the connection!
        this->_should_exit = true;
        return;
    }
    else if (opcode == "OACK") {
        //unimplemented
        return;
    }
    //Reaching the bottom of the list SHOULD be impossible, as we should have already made sure that either we
    //sent an error because the opcode didn't match anything, or it was one of the opcodes above.
    std::cerr << "packet_handler::handle_packet: opcode didn't match any known opcodes, but error packet was not transmitted." << std::endl;
    exit(1);
}
