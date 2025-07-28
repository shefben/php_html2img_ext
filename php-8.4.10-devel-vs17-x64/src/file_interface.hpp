#ifndef HTML2IMG_FILE_INTERFACE_HPP
#define HTML2IMG_FILE_INTERFACE_HPP

#include <RmlUi/Core.h>
#include <filesystem>
#include <cstdio>
#include "path_utils.hpp"

class PHPFileInterface : public Rml::FileInterface
{
public:
    explicit PHPFileInterface(const std::filesystem::path& base) : base_(base) {}

    Rml::FileHandle Open(const Rml::String& path) override {
        std::filesystem::path p = html2img::resolve_asset(path, base_, false);
        FILE* f = fopen(p.string().c_str(), "rb");
        return reinterpret_cast<Rml::FileHandle>(f);
    }

    void Close(Rml::FileHandle file) override {
        FILE* f = reinterpret_cast<FILE*>(file);
        if(f) fclose(f);
    }

    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override {
        FILE* f = reinterpret_cast<FILE*>(file);
        return fread(buffer, 1, size, f);
    }

    bool Seek(Rml::FileHandle file, long offset, int origin) override {
        FILE* f = reinterpret_cast<FILE*>(file);
        return fseek(f, offset, origin) == 0;
    }

    long Tell(Rml::FileHandle file) override {
        FILE* f = reinterpret_cast<FILE*>(file);
        return ftell(f);
    }

private:
    std::filesystem::path base_;
};

#endif // HTML2IMG_FILE_INTERFACE_HPP
