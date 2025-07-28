#ifndef HTML2IMG_RML_FILE_INTERFACE_HPP
#define HTML2IMG_RML_FILE_INTERFACE_HPP

#include <RmlUi/Core.h>
#include <filesystem>
#include <cstdio>

class RmlFileInterface : public Rml::FileInterface {
public:
    explicit RmlFileInterface(const std::filesystem::path& base);
    ~RmlFileInterface() override = default;

    Rml::FileHandle Open(const Rml::String& path) override;
    void Close(Rml::FileHandle file) override;
    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;
    bool Seek(Rml::FileHandle file, long offset, int origin) override;
    size_t Tell(Rml::FileHandle file) override;
    size_t Length(Rml::FileHandle file) override;

private:
    std::filesystem::path base_;
};

#endif // HTML2IMG_RML_FILE_INTERFACE_HPP
