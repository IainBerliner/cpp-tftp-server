#include "argument_handler/argument_handler.hpp"
#include "tftp_server/tftp_server.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv) {
    auto arguments = argument_handler::instance();
    arguments.process(argc, argv);

    #ifdef DEBUG_BUILD
    tftp_server server(arguments.get_int("port"), arguments.get_string("tftp-root"),
                       arguments.get_int("enable-reading"), arguments.get_int("enable-writing"),
                       arguments.get_int("allow-leaving-tftp-root"), arguments.get_double("drop"),
                       arguments.get_double("duplicate"), arguments.get_double("bitflip"));
    #else
    tftp_server server(arguments.get_int("port"), arguments.get_string("tftp-root"),
                       arguments.get_int("enable-reading"), arguments.get_int("enable-writing"),
                       arguments.get_int("allow-leaving-tftp-root"));
    #endif

    std::string message;
    if ((message = server.start()) != "success") {
        std::cout << message << std::endl;
        arguments.quit_with_usage_message();
    }

    server.join();

    return 0;
}
