#include "connection_handler.hpp"

#include "connection_options.hpp"
#include "../macros/constants.hpp"
#include "../networking/connection.hpp"
#ifdef DEBUG_BUILD
#include "../random/random.hpp"
#endif

#include <sys/stat.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef DEBUG_BUILD
void connection_handler::handle_connection(std::shared_ptr<std::atomic<bool>> server_running,
                                           connection ctn, const std::string tftp_root,
                                           int32_t mode, bool allow_leaving_tftp_root,
                                           double drop_rate, double duplicate_rate,
                                           double bitflip_rate) {
    connection_handler cnt_handler(server_running, ctn, tftp_root, mode, allow_leaving_tftp_root,
                                   drop_rate, duplicate_rate, bitflip_rate);
    cnt_handler._handle_connection();
    return;
}
#else
void connection_handler::handle_connection(std::shared_ptr<std::atomic<bool>> server_running,
                                           connection ctn, const std::string tftp_root,
                                           int32_t mode, bool allow_leaving_tftp_root) {
    connection_handler cnt_handler(server_running, ctn, tftp_root, mode, allow_leaving_tftp_root);
    cnt_handler._handle_connection();
    return;
}
#endif

void connection_handler::_handle_connection() {
    while (!this->close_connection) {
        if (!*(server_running.get())) return;
        try { this->respond_to_client(); }
        catch(tftp_error& err){ 
            #ifdef DEBUG_BUILD
            std::cout << err.what() << std::endl;
            #endif
            this->send_error_packet(err);
            break;
        }
    }
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::_handle_connection: closing connection." << std::endl;
    #endif
    
    return;
}

void connection_handler::respond_to_client(uint32_t timeouts) {
    int32_t result = ctn.receive_timeout(this->options.get_int("timeout"), 0);
    if (result == -1) { throw tftp_error("connection failed"); }
    if (result == -2) {
        if (timeouts > this->options.get_int("max_timeouts")) { 
            std::cout << "connection_handler::respond_to_client: max timeouts reached.";
            std::cout << std::endl; throw tftp_error("max timeouts reached");
        }
        else { this->respond_to_client(timeouts +  1); }
    }
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::respond_to_client: new packet is ";
    std::cout << (int)ctn.get_buffer().size() << " bytes long." << std::endl;
    #endif
    std::vector<uint8_t>& incoming_packet = ctn.get_buffer();
    #ifndef DEBUG_BUILD
    this->handle_packet(incoming_packet);
    #else
    flip_random_bits(incoming_packet, this->options.get_double("bitflip_rate"));
    if (!get_random_bool(this->options.get_double("drop_rate"))) {
        if (get_random_bool(this->options.get_double("duplicate_rate"))) { 
            this->handle_packet(incoming_packet); this->handle_packet(incoming_packet); 
        }
        else { this->handle_packet(incoming_packet); }
    }
    #endif
}

void connection_handler::handle_packet(std::vector<uint8_t>& tftp_packet) {
    uint16_t opcode = decode_opcode(tftp_packet);
    
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::handle_packet: op code: " << opcode << std::endl;
    #endif
    
    if (opcode == OP_RRQ) {
        this->handle_RRQ_packet(tftp_packet);
        return;
    }
    else if (opcode == OP_WRQ) {
        this->handle_WRQ_packet(tftp_packet);
        return;
    }
    else if (options.get_string("operation") == "") {
        /*
        //Send error packet if received packet is not RRQ or WRQ but the server has not already
        //received either of these
        */
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_packet: throwing tftp_error(5) because the client ";
        std::cout << "sent data/ack packet but server has not received RRQ/WRQ packet." << std::endl;
        #endif
        throw tftp_error(5);
        this->close_connection = true;
        return;
    }
    //Complementary to WRQ
    else if (opcode == OP_DATA) {
        this->handle_DATA_packet(tftp_packet);
        return;
    }
    //Complementary to RRQ
    else if (opcode == OP_ACK) {
        this->handle_ACK_packet(tftp_packet);
        return;
    }
    //Terminate the connection.
    else if (opcode == OP_ERROR) {
        this->close_connection = true;
        return;
    }
    //Client functionality not implemented.
    else if (opcode == OP_OACK) {
        return;
    }
    //If the end of the list is reached, an invalid opcode was provided.
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::handle_packet: throwing tftp_error(0) because opcode didn't ";
    std::cout << "match any known opcodes." << std::endl;
    #endif
    throw tftp_error("opcode unrecognized");
}

bool path_leaves_root(const std::filesystem::path& path) {
    std::string cur_path;
    for (std::string dir : path) {
        cur_path += "/" + dir;
        if (std::filesystem::weakly_canonical(cur_path) == "/" && dir == "..") { return true; }
    }
    return false;
}

void connection_handler::handle_RRQ_packet(std::vector<uint8_t>& tftp_packet) {
    if (!(options.get_int("mode") == MODE_RRQ
        || options.get_int("mode") == MODE_RRQ_AND_WRQ)) {
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_RRQ_packet: throwing tftp_error(2) because ";
        std::cout << "reading mode is not enabled." << std::endl;
        #endif
        throw tftp_error(2);
        return;
    }
    
    this->options.update(decode_request_header(tftp_packet));
    if (!options.get_int("allow_leaving_tftp_root")
        && path_leaves_root(this->options.get_string("rel_filepath"))) {
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_RRQ_packet: throwing tftp_error(2) because the ";
        std::cout << "path passed by client leaves tftp_root at some point along its resolution.";
        std::cout << std::endl;
        #endif
        throw tftp_error(2);
        return;
    }

    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::handle_RRQ_packet: tftp_root: ";
    std::cout << this->options.get_string("tftp_root") << std::endl;
    std::cout << "connection_handler::handle_RRQ_packet: rel_filepath: ";
    std::cout << this->options.get_string("rel_filepath") << std::endl;
    std::cout << "connection_handler::handle_RRQ_packet: transfer_mode: ";
    std::cout << this->options.get_string("transfer_mode") << std::endl;
    #endif
    
    this->options.set("operation", std::string("RRQ"));
    
    std::unordered_map<std::string, int32_t> requested_options = decode_request_header_options(tftp_packet);
    this->options.update(requested_options);
    this->open_file();
    
    if (requested_options.size() == 0) {
        /*
        //To simplify implementation, if an oack packet is not sent to the client,
        //then a fake ack packet with a block number of 0 is sent to the server,
        //as that is what the client ordinarily responds with if sent an oack packet.
        */
        std::vector<uint8_t> fake_ack_packet;
        this->append_network_short(fake_ack_packet, 4);
        this->append_network_short(fake_ack_packet, 0);
        this->handle_ACK_packet(fake_ack_packet);
    }
    else {
        const std::vector<uint8_t> oack_packet = this->create_oack_packet(requested_options);
        this->send(oack_packet);
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_RRQ_packet: sending oack packet to client to ";
        std::cout << "acknowledge RRQ." << std::endl;
        #endif
    }
    
    return;
}

void connection_handler::handle_WRQ_packet(std::vector<uint8_t>& tftp_packet) {
    if (!(options.get_int("mode") == MODE_WRQ
        || options.get_int("mode") == MODE_RRQ_AND_WRQ)) {
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_WRQ_packet: throwing tftp_error(2) because ";
        std::cout << "writing mode is not enabled." << std::endl;
        #endif
        throw tftp_error(2);
        return;
    }
    
    this->options.update(decode_request_header(tftp_packet));
    
    if (!options.get_int("allow_leaving_tftp_root")
        && path_leaves_root(this->options.get_string("rel_filepath"))) {
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_WRQ_packet: throwing tftp_error(2) because the ";
        std::cout << "path passed by client leaves tftp_root at some point along its resolution.";
        std::cout << std::endl;
        #endif
        throw tftp_error(2);
        return;
    }
    
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::handle_RRQ_packet: tftp_root: ";
    std::cout << this->options.get_string("tftp_root") << std::endl;
    std::cout << "connection_handler::handle_RRQ_packet: rel_filepath: ";
    std::cout << this->options.get_string("rel_filepath") << std::endl;
    std::cout << "connection_handler::handle_RRQ_packet: transfer_mode: ";
    std::cout << this->options.get_string("transfer_mode") << std::endl;
    #endif
    
    this->options.set("operation", std::string("WRQ"));

    //RFC-1350 recommends that mail mode not be implemented.
    if (this->options.get_string("transfer_mode") == "mail") {
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_WRQ_packet: throwing tftp_error(0) because mail ";
        std::cout << "mode is not implemented." << std::endl;
        #endif
        throw tftp_error("mail mode not implemented");
    }

    std::unordered_map<std::string, int32_t> requested_options = decode_request_header_options(tftp_packet);
    this->options.update(requested_options);
    this->open_file();
    
    if (requested_options.size() == 0) {
        const std::vector<uint8_t> ack_packet = this->create_ack_packet();
        this->send(ack_packet);
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_WRQ_packet: sending ack packet to client to ";
        std::cout << "acknowledge WRQ." << std::endl;
        #endif
    }
    else {
        const std::vector<uint8_t> oack_packet = this->create_oack_packet(requested_options);
        this->send(oack_packet);
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_WRQ_packet: sending oack packet to client to ";
        std::cout << "acknowledge WRQ." << std::endl;
        #endif
    }
    return;
}

void connection_handler::handle_DATA_packet(std::vector<uint8_t>& tftp_packet) {
    uint64_t block_number = connection_handler::decode_block_number(tftp_packet);
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::handle_DATA_packet: block_number: " << block_number;
    std::cout << std::endl;
    std::cout << "connection_handler::handle_DATA_packet: prev_block_number: ";
    std::cout << this->prev_block_number << std::endl;
    #endif
    if (block_number == this->prev_block_number + 1) {
        //increment prev_block_number
        this->prev_block_number = block_number;
        //write data
        if (!this->file.write_content(tftp_packet)) { throw tftp_error("file write failed"); }
        std::vector<uint8_t> ack_packet = this->create_ack_packet();
        this->send(ack_packet);
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_DATA_packet: sending ack packet to acknowledge ";
        std::cout << "having received data." << std::endl;
        #endif
        this->terminate_if_last_packet(tftp_packet);
        return;
    }
    else {
        //Ignore packets that are not in order.
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_DATA_packet: ignoring duplicate packet.";
        std::cout << std::endl;
        #endif
        return;
    }
}

void connection_handler::handle_ACK_packet(std::vector<uint8_t>& tftp_packet) {
    uint64_t block_number = connection_handler::decode_block_number(tftp_packet);
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::handle_ACK_packet: block_number: " << block_number;
    std::cout << std::endl;
    std::cout << "connection_handler::handle_ACK_packet: prev_block_number: ";
    std::cout << this->prev_block_number << std::endl;
    #endif
    
    if (block_number == this->prev_block_number + 1 || block_number == 0) {
        //increment prev_block_number
        this->prev_block_number = block_number;
        //read data
        std::vector<uint8_t> data_packet = this->create_data_packet();
        this->send(data_packet);
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_ACK_packet: sending data packet to client in ";
        std::cout << "response to ACK packet." << std::endl;
        #endif
        this->terminate_if_last_packet(data_packet);  
        return;
    }
    else {
        //Ignore packets that are not in order.
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::handle_ACK_packet: ignoring duplicate packet.";
        std::cout << std::endl;
        #endif
        return;
    }
}

uint16_t connection_handler::decode_opcode(std::vector<uint8_t>& tftp_packet) {
    if (tftp_packet.size() < 2) { throw tftp_error(4); }
    const uint16_t opcode = ntohs(*((uint16_t*)(&tftp_packet[0])));
    return opcode;
}

int64_t connection_handler::decode_block_number(std::vector<uint8_t>& tftp_packet) {
    //If there is no block number to decode, throw an exception.
    if (tftp_packet.size() < 4) { throw tftp_error(4); }
    uint16_t block_number = ntohs(*((uint16_t*)(&tftp_packet[2])));
    //Handle block number rollover
    if (block_number == 0 && this->prev_block_number != 0 ) {
        this->block_number_rollover_cnt++;
    }
    block_number = this->block_number_rollover_cnt * BLOCK_NUMBER_LIMIT + block_number;
    return block_number;
}

//Return rel_filepath and mode from decoded RRQ/WRQ header
std::unordered_map<std::string, std::string> connection_handler::decode_request_header(std::vector<uint8_t>& tftp_packet) {
    std::vector<int> zeroes = {};
    //ensure null-terminators for the rel_filepath and mode exist
    for (uint32_t i = 2; i < tftp_packet.size(); i++) {
        if(tftp_packet[i] == 0) { zeroes.push_back(i); }
        if (zeroes.size() == 2) { break; }
    }
    if (zeroes.size() < 2) { throw tftp_error(4); }
    
    //locate and return the rel_filepath and mode using the null-terminators
    std::string rel_filepath = (char *)(tftp_packet.data() + 2);
    std::string transfer_mode = (char *)(tftp_packet.data() + zeroes[0] + 1);
    to_lowercase(transfer_mode);
    
    return {{"rel_filepath", rel_filepath}, {"transfer_mode", transfer_mode}};
}

std::unordered_map<std::string, int32_t> connection_handler::decode_request_header_options(std::vector<uint8_t>& tftp_packet) {
    std::vector<int> zeroes = {};
    std::unordered_map<std::string, int32_t> options_map = {};
    
    for (uint32_t i = 2; i < tftp_packet.size(); i++) {
        if(tftp_packet[i] == 0) { zeroes.push_back(i); };
    }
    /*
    //Options (which are null terminated strings) should always come in pairs with their values
    //(which are null terminated strings). therefore, throw an exception if this is not the case.
    */
    if (zeroes.size() % 2 != 0) { throw tftp_error(4); }
    
    for (uint32_t i = 2; i < zeroes.size(); i = i + 2) {
        std::string option = (char*)&tftp_packet[zeroes[i - 1] + 1];
        to_lowercase(option);
        std::string value = (char*)&tftp_packet[zeroes[i] + 1];
        options_map[option] = std::stoi(value);
    }
    #ifdef DEBUG_BUILD
    for (auto i: options_map) {
        std::cout << "connection_handler::decode_request_header_options: option: ";
        std::cout << i.first << std::endl;
        std::cout << "connection_handler::decode_request_header_options: value: ";
        std::cout << i.second << std::endl;
    }
    #endif
    
    return options_map;
}
/*
//Adds "number" to the data packet, ensuring conversion from host short (big endian or little
//endian) to network (big endian) short.
*/
void connection_handler::append_network_short(std::vector<uint8_t>& data, const uint16_t number) {
    const uint16_t network_short = htons(number);
    const uint8_t* network_short_ptr = (const uint8_t*)&network_short;
    data.push_back(network_short_ptr[0]);
    data.push_back(network_short_ptr[1]);
    return;
}

void connection_handler::to_lowercase(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](uint8_t c){ return std::tolower(c); } );
}

std::vector<uint8_t> connection_handler::create_data_packet() {
    //Handle block number rollover
    const uint16_t block_number_truncated = (this->prev_block_number + 1) % BLOCK_NUMBER_LIMIT;
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::create_data_packet: block_number: " << (this->prev_block_number + 1) << std::endl;
    #endif
    std::vector<uint8_t> data = {};
    //Append data op code (03)
    append_network_short(data, 3);
    //Append block number
    append_network_short(data, block_number_truncated);
    //Append content
    if (!this->file.append_content(data)) { throw tftp_error("file read failed"); }
    return data;
}

std::vector<uint8_t> connection_handler::create_ack_packet() {
    //Handle block number rollover
    const uint16_t block_number_truncated = this->prev_block_number % BLOCK_NUMBER_LIMIT;
    std::vector<uint8_t> ack = {};
    //Append acknowledgement opcode (04)
    append_network_short(ack, 4);
    //Append block number
    append_network_short(ack, block_number_truncated);
    return ack;
}

std::vector<uint8_t> connection_handler::create_oack_packet(std::unordered_map<std::string, int32_t> requested_options) {
    std::vector<uint8_t> oack = {};
    //Append option acknowledgement opcode (06)
    append_network_short(oack, 6);
    //Append option string/value pairs for supported options
    for (auto& [str, value]: requested_options) {
        oack.insert(oack.end(), str.begin(), str.end());
        /*
        //get value from connection_options instead of requested_options, because tsize relies on
        //the connection_options class to set the correct value.
        */
        std::string value_as_string = std::to_string(this->options.get_int(str));
        oack.insert(oack.end(), value_as_string.begin(), value_as_string.end());
    }
    return oack;
}

std::vector<uint8_t> connection_handler::create_error_packet(tftp_error& error) {
    std::vector<uint8_t> err = {};
    //Append error opcode (05)
    append_network_short(err, 5);
    //Append error
    append_network_short(err, error.error_code);
    //Append error message
    std::string error_message = error.what();
    err.insert(err.end(), error_message.begin(), error_message.end());
    return err;
}

void connection_handler::send(const std::vector<uint8_t>& data) {
    this->ctn.send(data);
    prev_packet = data;
}

void connection_handler::resend_packet() {
    this->ctn.send(this->prev_packet);
    return;
}

void connection_handler::send_error_packet(tftp_error& error) {
    const std::vector<uint8_t> error_packet = this->create_error_packet(error);
    this->send(error_packet);
    #ifdef DEBUG_BUILD
    std::cout << "connection_handler::send_error_packet: sending error packet to client with ";
    std::cout << "message: " << error.what() << std::endl;
    #endif
    this->close_connection = true;
    return;
}

void connection_handler::terminate_if_last_packet(std::vector<uint8_t>& packet) {
    if ((packet.size() - 4) < this->options.get_int("blksize")) { this->close_connection = true; }
}

void connection_handler::open_file() {
    std::string filepath = options.get_string("tftp_root") + "/"
                                              + options.get_string("rel_filepath");
    //If sending a file to the client, check if the file exists first
    struct stat buffer; 
    if (options.get_string("operation") == "RRQ" && stat(filepath.c_str(), &buffer) != 0) {
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::open_file: throwing tftp_error(1) because the file does ";
        std::cout << "not exist." << std::endl;
        #endif
        throw tftp_error(1);
    }
    //Open the file
    if (!this->file.open(filepath, options.get_int("blksize"),
                         options.get_string("transfer_mode") == "netascii",
                         options.get_string("operation") == "WRQ")) {
        #ifdef DEBUG_BUILD
        std::cout << "connection_handler::open_file: throwing tftp_error(2) because the file did ";
        std::cout << "not open successfully." << std::endl;
        #endif
        throw tftp_error(2);
    }
}
