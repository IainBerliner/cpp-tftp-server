#ifndef PACKET_HANDLING_MACROS_HPP
#define PACKET_HANDLING_MACROS_HPP
#ifdef TESTING_BUILD

#define CATCH_INPUT_VALIDATION_ERRORS(input_expression)                                                                                          \
try {                                                                                                                                            \
input_expression;                                                                                                                                \
} catch (const int error_number) {                                                                                                               \
    const std::vector<unsigned char> error_packet = create_error_packet(error_number);                                                           \
    send_packet(error_packet, client_sockfd);                                                                                                    \
    last_sent_packet = error_packet;                                                                                                             \
    std::cout << "server: sending error packet to client because a function that parses a packet threw a developer defined error." << std::endl; \
    std::cout << "server: the error sent was: " << TFTP_ERRORS.at(error_number) << std::endl;                                                    \
    this->_should_exit = true;                                                                                                                   \
    return;                                                                                                                                      \
} catch (...) {                                                                                                                                  \
    const std::vector<unsigned char> error_packet = create_error_packet(4);                                                                      \
    send_packet(error_packet, client_sockfd);                                                                                                    \
    last_sent_packet = error_packet;                                                                                                             \
    std::cout << "server: sending error packet to client because a function that parses a packet threw a generic error." << std::endl;           \
    std::cout << "server: the error sent was: " << TFTP_ERRORS.at(4) << std::endl;                                                               \
    this->_should_exit = true;                                                                                                                   \
    return;                                                                                                                                      \
}

#else

#define CATCH_INPUT_VALIDATION_ERRORS(input_expression)                                \
try {                                                                                  \
input_expression;                                                                      \
} catch (const int error_number) {                                                     \
    const std::vector<unsigned char> error_packet = create_error_packet(error_number); \
    send_packet(error_packet, client_sockfd);                                          \
    last_sent_packet = error_packet;                                                   \
    this->_should_exit = true;                                                         \
    return;                                                                            \
} catch (...) {                                                                        \
    const std::vector<unsigned char> error_packet = create_error_packet(4);            \
    send_packet(error_packet, client_sockfd);                                          \
    last_sent_packet = error_packet;                                                   \
    this->_should_exit = true;                                                         \
    return;                                                                            \
}
#endif
#endif
