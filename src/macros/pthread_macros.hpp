#ifndef PTHREAD_MACROS_HPP
#define PTHREAD_MACROS_HPP

#include <atomic>
#include <iostream>
#include <memory>
#include <tuple>
#include <vector>

#include "../handle_connection/handle_connection.hpp"

#define DELETE_HANDLE_CONNECTION_INPUT_STRUCT 0
#define CLOSE_SOCKFD 1
#define NOTIFY_CHILDREN_OF_DEATH 2

inline void pthread_cleanup_wrapper(void* input_pointer) {
    const std::vector<std::tuple<int, void*>> cleanup_wrapper_stack = *(std::vector<std::tuple<int, void*>>*)(input_pointer);
    for (int i = 0; i < cleanup_wrapper_stack.size(); i++) {
        if (std::get<0>(cleanup_wrapper_stack[i]) == DELETE_HANDLE_CONNECTION_INPUT_STRUCT) {
            delete *(handle_connection_input_struct**)std::get<1>(cleanup_wrapper_stack[i]);
        }
        else if (std::get<0>(cleanup_wrapper_stack[i]) == CLOSE_SOCKFD) {
            close(*(int*)std::get<1>(cleanup_wrapper_stack[i]));
        }
        else if (std::get<0>(cleanup_wrapper_stack[i]) == NOTIFY_CHILDREN_OF_DEATH) {
            std::shared_ptr<std::atomic<bool>> parent_thread_alive = *(std::shared_ptr<std::atomic<bool>>*)std::get<1>(cleanup_wrapper_stack[i]);
            *parent_thread_alive = false;
        }
    }
}

//This should be placed at the start of any thread in this repository.
//It disables thread cancelability and makes it so that if the thread were cancellable,
//It would only be cancelled when a blocking call is made, not instantaneously.
#define THREAD_START()                                                               \
pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);                                \
pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);                                \
thread_local std::vector<std::tuple<int, void*>> pthread_cleanup_wrapper_stack = {}; \
pthread_cleanup_push(pthread_cleanup_wrapper, (void*)&pthread_cleanup_wrapper_stack);

//This macro registers a thread_cancellation_handler function to call free on a specific pointer.
//which may be used to clean up a dynamically allocated pointer when the thread exits without returning.
#define THREAD_REGISTER_CLEANUP(type, value) pthread_cleanup_wrapper_stack.push_back(std::make_tuple(type, (void*)(&value)));
//Wrap this around any blocking call that is desirable to specify as a cancellation point.
//This is needed because, if cancellation is started within the destructor of an object,
//then an error will be raised; this is actually, behind the scenes, how thread cancellation works in c++11 and above for most compilers.
//
//This is a problem because destructors MUST NOT raise raise errors, because they tend to be called when other errors are being handled,
//and if any destructor calls a blocking function that pthread specifies as a cancellation point then, because pthread_cancel actually uses exceptions,
//an error will be raised within a catch clause, which causes immediate program exit.
//
//Some solutions online suggest the usage of semaphores to solve this issue, but I couldn't really figure out how to do that elegantly.
#define THREAD_SPECIFY_CANCELLATION_POINT(input_statement) \
pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);       \
input_statement;                                           \
pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#endif

#define THREAD_CLEANUP() \
pthread_cleanup_pop(1);
