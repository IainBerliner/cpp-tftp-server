#ifndef NETASCII_HPP
#define NETASCII_HPP
#include <vector>
std::vector<unsigned char> to_net_ascii(std::vector<unsigned char> input_byte_vector, const int start_index, const int end_index);
std::vector<unsigned char> from_net_ascii(std::vector<unsigned char> input_byte_vector, unsigned int start_index, unsigned int end_index);
#endif
