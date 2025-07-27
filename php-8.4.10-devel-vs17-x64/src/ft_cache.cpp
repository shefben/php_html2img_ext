#include "ft_cache.hpp"
#include <stdexcept>

FtCache::FtCache() {
    if (FT_Init_FreeType(&lib_)) {
        throw std::runtime_error("Failed to init FreeType");
    }
}

FtCache::~FtCache() {
    for (auto &p : cache_) {
        FT_Done_Face(p.second);
    }
    FT_Done_FreeType(lib_);
}

FT_Face FtCache::load(const std::filesystem::path &path) {
    auto it = cache_.find(path.string());
    if (it != cache_.end()) {
        return it->second;
    }
    FT_Face face;
    if (FT_New_Face(lib_, path.string().c_str(), 0, &face)) {
        return nullptr;
    }
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    cache_[path.string()] = face;
    return face;
}
