#include "cache.hpp"
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <fstream>

std::string cache_key(const std::string &html, const std::string &format)
{
    std::string data = html + format + "html2img-0.1";
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);
    static const char hex[] = "0123456789abcdef";
    std::string out; out.reserve(SHA_DIGEST_LENGTH*2);
    for(unsigned char c: digest) {
        out.push_back(hex[c>>4]);
        out.push_back(hex[c&0xF]);
    }
    return out;
}

std::filesystem::path default_cache_dir()
{
#ifdef _WIN32
    const char* tmp = getenv("TEMP");
    if(!tmp) tmp = "C:\\Temp";
    return std::filesystem::path(tmp) / "php-html2img-cache";
#else
    const char* tmp = getenv("TMPDIR");
    if(!tmp) tmp = "/tmp";
    return std::filesystem::path(tmp) / "php-html2img-cache";
#endif
}

void ensure_directory(const std::filesystem::path &dir)
{
    std::error_code ec;
    if(!std::filesystem::exists(dir, ec)) {
        std::filesystem::create_directories(dir, ec);
    }
}

FileLock::FileLock(const std::filesystem::path &file)
{
#ifdef _WIN32
    handle_ = CreateFileW(std::wstring(file.wstring()).c_str(), GENERIC_READ|GENERIC_WRITE,
                          FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    OVERLAPPED ov{};
    LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov);
#else
    fd_ = open(file.c_str(), O_RDWR | O_CREAT, 0666);
    if(fd_ >= 0) {
        ::flock(fd_, LOCK_EX);
    }
#endif
}

FileLock::~FileLock()
{
#ifdef _WIN32
    if(handle_) {
        OVERLAPPED ov{};
        UnlockFileEx(handle_, 0, MAXDWORD, MAXDWORD, &ov);
        CloseHandle(handle_);
    }
#else
    if(fd_ >= 0) {
        ::flock(fd_, LOCK_UN);
        close(fd_);
    }
#endif
}
