#ifndef HTML2IMG_CACHE_HPP
#define HTML2IMG_CACHE_HPP

#include <string>
#include <filesystem>

std::string cache_key(const std::string &html, const std::string &format);

std::filesystem::path default_cache_dir();
void ensure_directory(const std::filesystem::path &dir);

class FileLock
{
public:
    explicit FileLock(const std::filesystem::path &file);
    ~FileLock();

    FileLock(const FileLock &)            = delete;
    FileLock &operator=(const FileLock &) = delete;

private:
    int fd_{-1};                  //  ←  add / keep this line
};

#endif
