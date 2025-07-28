#include "cache.hpp"
#include <fcntl.h>
#include <sys/stat.h>

#if defined(_WIN32)
/* ── Windows replacements ───────────────────────────────────────────── */
  #include <windows.h>
  #include <io.h>          // _open, _close, _read …
  #include <share.h>       // _SH_DENYNO
#else
  #include <sys/file.h>    // flock()
  #include <unistd.h>
#endif

#include <chrono>
#include <cstring>
#include <fstream>
#include <filesystem>


/* ------------------------------------------------------------------ */
/*  Cross‑platform file‑locking helpers                               */
/* ------------------------------------------------------------------ */
static void lock_file(int fd_or_handle, bool exclusive = true)
{
#ifdef _WIN32
    OVERLAPPED ovl{};
    auto h = reinterpret_cast<HANDLE>(_get_osfhandle(fd_or_handle));
    DWORD flags = exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    LockFileEx(h, flags, 0, /*bytesLow*/1, /*bytesHigh*/0, &ovl);
#else
    flock(fd_or_handle, exclusive ? LOCK_EX : LOCK_SH);
#endif
}

static void unlock_file(int fd_or_handle)
{
#ifdef _WIN32
    OVERLAPPED ovl{};
    auto h = reinterpret_cast<HANDLE>(_get_osfhandle(fd_or_handle));
    UnlockFileEx(h, 0, /*bytesLow*/1, /*bytesHigh*/0, &ovl);
#else
    flock(fd_or_handle, LOCK_UN);
#endif
}


/* ------------------------------------------------------------------ */
/*  Tiny dependency‑free hash – FNV‑1a 64‑bit                          */
/* ------------------------------------------------------------------ */
static uint64_t fnv1a_64(const void* data, size_t len)
{
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0xcbf29ce484222325ull;          // offset basis
    for(size_t i=0;i<len;++i){
        h ^= p[i];
        h *= 0x100000001b3ull;                  // FNV prime
    }
    return h;
}

std::string cache_key(const std::string &html, const std::string &format)
{
    std::string data = html + format + "html2img-0.1";
    uint64_t h = fnv1a_64(data.data(), data.size());

    /* hex‑encode the 64‑bit result */
    static const char hex[] = "0123456789abcdef";
    std::string out; out.reserve(16);
    for(int i=56;i>=0;i-=8){
        uint8_t byte = static_cast<uint8_t>(h >> i);
        out.push_back(hex[byte >> 4]);
        out.push_back(hex[byte & 0xF]);
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
    fd_ = _open(file.string().c_str(), _O_RDWR | _O_CREAT,
                _S_IREAD | _S_IWRITE);
#else
    fd_ = ::open(file.c_str(), O_RDWR | O_CREAT, 0666);
#endif
    if (fd_ >= 0)
        lock_file(fd_, /*exclusive*/true);
}

FileLock::~FileLock()
{
    if (fd_ >= 0)
    {
        unlock_file(fd_);
#ifdef _WIN32
        _close(fd_);
#else
        ::close(fd_);
#endif
    }
}

