#ifndef NETWORKING_HPP
#define NETWORKING_HPP
#include <atomic>
#include <memory>
#ifndef TESTING_BUILD
int tftp_listen(const char* port, const char* tftp_root, const char* permitted_operations, const bool allow_leaving_tftp_root, std::shared_ptr<std::atomic<bool>> running);
#else
int tftp_listen(const char* port, const char* tftp_root, const char* permitted_operations, const bool allow_leaving_tftp_root,
		const float drop, const float duplicate, const float bitflip, std::shared_ptr<std::atomic<bool>> running);
#endif
int recvtimeout(int s, char *buf, int len, int flags, int timeout_sec, int timeout_usec);
#endif
