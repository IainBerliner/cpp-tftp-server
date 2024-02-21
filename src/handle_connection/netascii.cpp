#include "netascii.hpp"

#include <iostream>
#include <vector>

std::vector<unsigned char> to_net_ascii(std::vector<unsigned char> input_byte_vector, const int start_index, const int end_index) {
    std::vector<unsigned char> output_byte_vector = {};
    for (int i = start_index; i < end_index; i++) {
        //Ignore disallowed chars
        if (!((input_byte_vector[i] >= 32) && (input_byte_vector[i] <= 126)) && !((input_byte_vector[i] >= 7) && (input_byte_vector[i] <= 13)) && !(input_byte_vector[i] == 0)) {
            continue;
        }
        //Put \r before \n
        else if (input_byte_vector[i] == '\n') {
            output_byte_vector.push_back('\r');
            output_byte_vector.push_back('\n');
        } 
        //Put \0 after \r
        else if (input_byte_vector[i] == '\r') {
            output_byte_vector.push_back('\r');
            output_byte_vector.push_back('\0');
        }
        //Otherwise just append the character
        else {
            output_byte_vector.push_back(input_byte_vector[i]);
        }
    }
    //Return filtered output
    return output_byte_vector;
}

std::vector<unsigned char> from_net_ascii(std::vector<unsigned char> input_byte_vector, unsigned int start_index, unsigned int end_index) {
    std::vector<unsigned char> output_byte_vector = {};
    //static member: it persists between calls. This is important if an \r\n or \r\0 is perfectly split between packets.
    //since writing to a file is a server-side operation, complicated logic to recombine packets into 512 bytes (or the transfer size) is not needed.
    //thread_local is also very important, so different threads don't interfere with each other.
    thread_local static unsigned char last_read_byte = 0;
    //Carry over carriage return from end of previous packet
    if (last_read_byte == '\r') {
        output_byte_vector.push_back('\r');
    }
    //For unix: convert \r\n to \n and convert \r\0 to \r
    for (int i = start_index; i < end_index; i++) {
        if (input_byte_vector[i] == (unsigned char)'\n' && last_read_byte == '\r') {
            if (output_byte_vector.size() > 0) {
                output_byte_vector.pop_back();
            }
            output_byte_vector.push_back('\n');
            last_read_byte = (unsigned char)'\n';
        }
        else if (input_byte_vector[i] == (unsigned char)'\0' && last_read_byte == '\r') {
            if (output_byte_vector.size() > 0) {
                output_byte_vector.pop_back();
            }
            output_byte_vector.push_back('\r');
            //Below line prevents the null terminated \r from being read as the start of \r\n
            last_read_byte = 0;
        }
        else {
            //Special case: this \r can't effectively be removed from next packet if it's \r\n, if it's included in this packet.
            //So, just don't include it here and be sure to include it in the next packet.
            if (i == end_index - 1 && input_byte_vector[i] == '\r') {
                last_read_byte = '\r';
            }
            else {
                output_byte_vector.push_back(input_byte_vector[i]);
                last_read_byte = input_byte_vector[i];
            }
        }
    }
    return output_byte_vector;
}
