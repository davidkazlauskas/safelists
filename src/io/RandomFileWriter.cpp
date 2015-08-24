
#include <fstream>

#include "RandomFileWriter.hpp"

namespace {

    void fallocateFile(const char* path,int64_t size) {
        // todo, optimize with fallocate for linux
        std::ofstream file(path);
        file.seekp(size - 1);
        file.write("",1);
    }

}

namespace SafeLists {

RandomFileWriteHandle::RandomFileWriteHandle(const char* path,int64_t size)
    : _path(path), _size(size)
{
    // could be more optimized...
    std::ifstream exists(_path.c_str());
    if (!exists.is_open()) {
        fallocateFile(_path.c_str(),_size);
    }

    _handle = fopen(_path.c_str(),"r+");
}

RandomFileWriteHandle::~RandomFileWriteHandle() {
    fclose(_handle);
}

}
