#include "path_resolution.hpp"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <vector>

std::vector<std::string> split_string(const std::string input_string, char delimiter) {
    int i = 0;
    std::string new_string = "";
    std::vector<std::string> result;
    for (int i = 0; i < input_string.size(); i++) {
        if(input_string[i] == delimiter) {
            if (new_string != "") {
                result.push_back(new_string);
            }
            new_string = "";
        }
        else {
            new_string += input_string[i];
        }
    }
    if (new_string != "") {
        result.push_back(new_string);
    }
    return result;
}

bool path_is_directory (const std::string input_path) {
    struct stat* path_info = (struct stat*)malloc(sizeof(struct stat));
    int result = stat(input_path.c_str(), path_info);
    if (result == -1) {
        free(path_info);
        //can't be a directory if the path is invalid
        return false;
    }
    else {
        bool is_directory = ((path_info->st_mode & S_IFDIR) == S_IFDIR) ? true : false;
        free(path_info);
        return is_directory;
    }
}

std::string resolve_path (const std::string input_path) {
    char* resolved_path_as_c_string = (char*)malloc(ARBITRARY_PATH_SIZE_LIMIT);
    char* result = realpath(input_path.c_str(), resolved_path_as_c_string);
    if (result == NULL) {
        free(resolved_path_as_c_string);
        if (errno == EACCES) {
            return std::string("ACCESS VIOLATION");
        }
        else {
            return std::string("INVALID PATH");
        }
    }
    else {
        std::string resolved_path(resolved_path_as_c_string);
        free(resolved_path_as_c_string);
        if (path_is_directory(resolved_path)) {
            resolved_path += "/";
        }
        return resolved_path;
    }
}

bool file_is_subdirectory (const std::string canonical_input_directory, const std::string test_path) {
    //If the resolved test_path violates access or is invalid return false.
    std::string canonical_test_path = resolve_path(test_path);
    if (canonical_test_path == "ACCESS VIOLATION") {
    	#ifdef TESTING_BUILD
        std::cerr << "file_is_subdirectory: the canonical test path violates access privileges." << std::endl;
        #endif
        return false;
    }
    else if (canonical_test_path == "INVALID PATH") {
    	#ifdef TESTING_BUILD
        std::cerr << "file_is_subdirectory: the canonical test path is invalid." << std::endl;
        #endif
        return false;
    }
    //If the size of the canonical test_path is smaller than the size of the canonical_root_directory,
    //it logically cannot be a subdirectory (this also ensures the code that follows is valid).
    else if (canonical_test_path.size() < canonical_input_directory.size()) {
        return false;
    }
    //If any characters from the canonical_input_directory do not match the beginning of the canonical_test_path,
    //then it cannot be a subdirectory. If all of them do match, then it logically must be a subdirectory.
    for (int i = 0; i < canonical_input_directory.size(); i++) {
        if (!(canonical_input_directory[i] == canonical_test_path[i])) {
            return false;
        }
    }
    //If we made it through all of the above checks then return true
    return true;
}

//Overloaded version that is passed flag to be told if the path is invalid.
bool file_is_subdirectory (const std::string canonical_input_directory, const std::string test_path, bool& path_invalid) {
    //If the resolved test_path violates access or is invalid return false.
    std::string canonical_test_path = resolve_path(test_path);
    if (canonical_test_path == "ACCESS VIOLATION") {
    	#ifdef TESTING_BUILD
        std::cerr << "file_is_subdirectory: the canonical test path violates access privileges." << std::endl;
        #endif
        return false;
    }
    else if (canonical_test_path == "INVALID PATH") {
    	#ifdef TESTING_BUILD
        std::cerr << "file_is_subdirectory: the canonical test path is invalid." << std::endl;
        #endif
        path_invalid = true;
        return false;
    }
    //If the size of the canonical test_path is smaller than the size of the canonical_root_directory,
    //it logically cannot be a subdirectory (this also ensures the code that follows is valid).
    else if (canonical_test_path.size() < canonical_input_directory.size()) {
        return false;
    }
    //If any characters from the canonical_input_directory do not match the beginning of the canonical_test_path,
    //then it cannot be a subdirectory. If all of them do match, then it logically must be a subdirectory.
    for (int i = 0; i < canonical_input_directory.size(); i++) {
        if (!(canonical_input_directory[i] == canonical_test_path[i])) {
            return false;
        }
    }
    //If we made it through all of the above checks then return true
    return true;
}

//Why not simply compare canonical_input_directory and the canonical_input_path?
//Answer: people might be able to use relative paths to learn the names of directories outside tftp_root.
//For instance, they could submit a request to read a file "../some_directory_name/../tftp_root_directory_name/valid_file". If they get an error "INVALID PATH" they would know that
//the file name they tested for does not exist. If the request succeeds, they would know that some_directory_name is a valid directory.

//By walking along the path sequentially, instead of directly comparing resolved paths, such an exploit is prevented.
bool path_leaves_directory (const std::string input_directory, const std::string input_path) {
    //convert input directory to its canonical form
    const std::string canonical_input_directory = resolve_path(input_directory);
    //Return true just to be safe in case the input_directory is somehow invalid
    if (canonical_input_directory == "ACCESS VIOLATION" || canonical_input_directory == "INVALID PATH") {
        return true;
    }
    //Test if relative path ever leaves a given directory, even temporarily, by breaking it up into it's constituent components
    std::vector<std::string> path_components = split_string(input_path, '/');
    std::string current_path = canonical_input_directory;
    #ifdef TESTING_BULD
    std::cout << "path_leaves_directory: the current path: " << current_path << std::endl;
    std::cout << "path_leaves_directory: path components size: " << path_components.size() << std::endl;
    #endif
    bool path_invalid = false;
    for (int i = 0; i < path_components.size(); i++) {
        current_path += path_components[i];
        current_path = resolve_path(current_path);
        #ifdef TESTING_BUILD
        std::cout << "path_leaves_directory: the current path: " << current_path << std::endl;
        #endif
        if (!file_is_subdirectory(canonical_input_directory, current_path, path_invalid)) {
            if (path_invalid == true) {
                return false;
            }
            else {
                return true;
            }
        }
    }
    return false;
}
