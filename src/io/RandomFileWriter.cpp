
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
    {
        // could be more optimized...
        std::ifstream exists(_path.c_str(),std::ifstream::ate | std::ifstream::binary);
        if (!exists.is_open()) {
            fallocateFile(_path.c_str(),_size);
        } else {
            // ensure size is good
            int64_t realSize = exists.tellg();
            if (realSize != _size) {
                throw RandomFileWriterDifferentFileSizeException();
            }
        }
    }

    _handle = fopen(_path.c_str(),"r+");
}

RandomFileWriteHandle::~RandomFileWriteHandle() {
    fclose(_handle);
}

void RandomFileWriteHandle::write(const char* buffer,int64_t start,int64_t size) {
    if (start < 0) {
        throw RandomFileWriterOutOfBoundsWriteException();
    }

    if (start + size > _size) {
        throw RandomFileWriterOutOfBoundsWriteException();
    }

    ::fseek(_handle,0,start);
    ::fwrite(buffer,size,1,_handle);
}

void RandomFileWriteHandle::read(char* buffer,int64_t start,int64_t size) {

}


}
