#pragma once

#include <unistd.h>

namespace utils {

class FileDescriptor {
public:

    FileDescriptor();
    explicit FileDescriptor(int fd, bool owned = true);
    FileDescriptor(FileDescriptor&& other) noexcept;
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;
    ~FileDescriptor();

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    bool isValid() const noexcept;
    int get() const noexcept;
    void close();
    void reset(int fd = -1, bool owned = true);
    int release() noexcept;

private:
    int _fd;
    bool _owned;
};

}
