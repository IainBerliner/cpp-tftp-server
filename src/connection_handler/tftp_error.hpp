#pragma once

#include <string>
#include <vector>

class tftp_error: std::exception {
    const std::vector<const char*> errors = \
    {
     "Not Defined","File Not Found","Access Violation","Disk Full or Allocation Exceeded",
     "Illegal TFTP Operation","Unknown Transfer TID","File Already Exists","No Such User",
     "Option Request Denied"
    };
    std::string custom_msg;
public:
    uint32_t error_code;
    tftp_error(uint32_t error_code) {
        if (error_code < errors.size()) { this->error_code = error_code; }
        else { this->error_code = 0; }
    }
    tftp_error(const std::string& error_msg) {
        custom_msg = error_msg;
        error_code = 0;
    }
    ~tftp_error() noexcept { }
    const char* what() const noexcept {
        if (error_code == 0) { return custom_msg.c_str(); }
        else { return this->errors[error_code]; }
    }
};
