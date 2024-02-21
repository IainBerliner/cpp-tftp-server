#ifndef PATH_FUNCTIONS_HPP
#define PATH_FUNCTIONS_HPP
#include <string>

#include "../../macros/constants.hpp"

std::string resolve_path(const std::string input_path);
bool path_leaves_directory (const std::string input_directory, const std::string input_path);
#endif
