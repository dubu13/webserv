#include "utils/FileDescriptor.hpp"
#include "utils/Logger.hpp"

namespace utils {

FileDescriptor::FileDescriptor() : _fd(-1), _owned(true) {}

FileDescriptor::FileDescriptor(int fd, bool owned) : _fd(fd), _owned(owned) {}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept 
    : _fd(other._fd), _owned(other._owned) {
    other._fd = -1;
    other._owned = false;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
    if (this != &other) {
        close();
        _fd = other._fd;
        _owned = other._owned;
        other._fd = -1;
        other._owned = false;
    }
    return *this;
}

FileDescriptor::~FileDescriptor() { 
    close(); 
}

bool FileDescriptor::isValid() const noexcept { 
    return _fd >= 0; 
}

int FileDescriptor::get() const noexcept { 
    return _fd; 
}

void FileDescriptor::close() {
    if (isValid() && _owned) {
        ::close(_fd);
        _fd = -1;
    }
}

void FileDescriptor::reset(int fd, bool owned) {
    close();
    _fd = fd;
    _owned = owned;
}

int FileDescriptor::release() noexcept {
    int fd = _fd;
    _fd = -1;
    _owned = false;
    return fd;
}

} // namespace utils
