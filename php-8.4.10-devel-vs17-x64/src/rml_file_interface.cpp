#include "rml_file_interface.hpp"
#include "path_utils.hpp"
#include <vector>

using namespace std;

RmlFileInterface::RmlFileInterface(const filesystem::path& base)
    : base_(base) {}

Rml::FileHandle RmlFileInterface::Open(const Rml::String& path)
{
    filesystem::path full = html2img::resolve_asset(path.c_str(), base_, false);
    FILE* f = fopen(full.string().c_str(), "rb");
    return reinterpret_cast<Rml::FileHandle>(f);
}

void RmlFileInterface::Close(Rml::FileHandle file)
{
    if(file)
        fclose(reinterpret_cast<FILE*>(file));
}

size_t RmlFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file)
{
    return fread(buffer, 1, size, reinterpret_cast<FILE*>(file));
}

bool RmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
    return fseek(reinterpret_cast<FILE*>(file), offset, origin) == 0;
}

size_t RmlFileInterface::Tell(Rml::FileHandle file)
{
    return ftell(reinterpret_cast<FILE*>(file));
}

size_t RmlFileInterface::Length(Rml::FileHandle file)
{
    long pos = ftell(reinterpret_cast<FILE*>(file));
    fseek(reinterpret_cast<FILE*>(file), 0, SEEK_END);
    long len = ftell(reinterpret_cast<FILE*>(file));
    fseek(reinterpret_cast<FILE*>(file), pos, SEEK_SET);
    return static_cast<size_t>(len);
}
