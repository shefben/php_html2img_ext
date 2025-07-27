#ifndef HTML2IMG_FT_CACHE_HPP
#define HTML2IMG_FT_CACHE_HPP

#include <map>
#include <string>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H

class FtCache {
public:
    FtCache();
    ~FtCache();

    FT_Face load(const std::filesystem::path &path);
private:
    FT_Library lib_{};
    std::map<std::string, FT_Face> cache_;
};

#endif
