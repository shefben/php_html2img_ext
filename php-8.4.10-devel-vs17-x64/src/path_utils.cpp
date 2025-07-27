#include "path_utils.hpp"
#include <algorithm>

namespace html2img {

static inline std::string strip_prefix(const std::string &s,
                                       const std::string &pfx)
{
    if(s.rfind(pfx, 0) == 0) {
        return s.substr(pfx.size());
    }
    return s;
}

std::filesystem::path from_file_uri(const std::string &uri)
{
    if(uri.rfind("file://", 0) != 0) {
        return std::filesystem::path(uri);
    }
    std::string p = uri.substr(7);
    if(!p.empty() && p[0] == '/') {
#ifdef _WIN32
        // Handle file:///C:/path  ->  C:/path
        if(p.size() >= 3 && p[2] == ':') {
            p.erase(0, 1);
        }
#endif
    }
    return std::filesystem::path(p);
}

std::filesystem::path resolve_asset(const std::string &src,
                                    const std::filesystem::path &base,
                                    bool allow_remote)
{
    if(src.find("://") != std::string::npos && src.rfind("file://", 0) != 0) {
        return allow_remote ? std::filesystem::path(src) : std::filesystem::path();
    }
    auto p = from_file_uri(src);
    if(p.is_absolute()) {
        return p;
    }
    return base / p;
}

} // namespace html2img
