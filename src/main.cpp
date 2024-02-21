#include <unistd.h>

#include <iostream>
#include <string>

#include "argument_handling/handle_arguments.hpp"
#include "tftp_server/tftp_server.hpp"

int main(int argc, char** argv) {

    arguments_struct arguments = get_default_arguments();    
    
    handle_arguments(argc, argv, arguments);
    
    #ifndef TESTING_BUILD
    tftp_server server(arguments.port, arguments.tftp_root, arguments.ops, arguments.allow_leaving_tftp_root);
    #else
    tftp_server server(arguments.port, arguments.tftp_root, arguments.ops, arguments.allow_leaving_tftp_root,
                                           arguments.drop, arguments.duplicate, arguments.bitflip);
    #endif
    
    std::string rv;
    if ((rv = server.start()) != "success") {
    	std::cout << rv << std::endl;
    	return 1;
    }
    
    pthread_join(server.sockfd(), NULL);
    
    return 0;
}
