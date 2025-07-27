#ifndef HTML2IMG_CACHE_HPP
#define HTML2IMG_CACHE_HPP

#include <string>
#include <filesystem>

std::string cache_key(const std::string &html, const std::string &format);

std::filesystem::path default_cache_dir();
void ensure_directory(const std::filesystem::path &dir);

class FileLock {
public:
    explicit FileLock(const std::filesystem::path &file);
    ~FileLock();
private:
#ifdef _WIN32
    void *handle_{}; // HANDLE
#else
    int fd_{-1};
#endif
};

#endif
