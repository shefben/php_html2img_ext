#ifndef HTML2IMG_PATH_UTILS_HPP
#define HTML2IMG_PATH_UTILS_HPP

#include <string>
#include <filesystem>

namespace html2img {

// Convert file:/// URI to filesystem path.
// Non-file URIs are returned unchanged as paths.
std::filesystem::path from_file_uri(const std::string &uri);

// Resolve an asset path against a base directory.
// Returns absolute path; empty path if the input is a remote URI.
std::filesystem::path resolve_asset(const std::string &src,
                                    const std::filesystem::path &base,
                                    bool allow_remote = false);

}

#endif // HTML2IMG_PATH_UTILS_HPP
