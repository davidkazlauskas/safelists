
#include <fstream>
#include <templatious/FullPack.hpp>

#include "RandomFileWriter.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    void fallocateFile(const char* path,int64_t size) {
        // todo, optimize with fallocate for linux
        std::ofstream file(path);
        file.seekp(size - 1);
        file.write("",1);
    }

    bool fileRangeCheck(int64_t start,int64_t size,int64_t max) {
        if (start < 0) {
            return false;
        }

        if (start + size > max) {
            return false;
        }

        return true;
    }

    template <class Vec,class Func>
    void traverseVecStartingAt(Vec& vec,Func&& func,int start,int step = -1) {
        int iterations = SA::size(vec) / std::abs(step);
        int current = start;
        TEMPLATIOUS_REPEAT( iterations ) {
            bool res = func(SA::getByIndex(vec,current));
            if (!res) {
                return;
            }
            current += step;
        }
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
    if (!fileRangeCheck(start,size,_size)) {
        throw RandomFileWriterOutOfBoundsWriteException();
    }

    ::fseek(_handle,0,start);
    ::fwrite(buffer,size,1,_handle);
}

void RandomFileWriteHandle::read(char* buffer,int64_t start,int64_t size) {
    if (!fileRangeCheck(start,size,_size)) {
        throw RandomFileWriterOutOfBoundsWriteException();
    }

    ::fseek(_handle,0,start);
    ::fread(buffer,size,1,_handle);
}

RandomFileWriteCache::RandomFileWriteCache(int items)
    : _maxItems(items)
{

}

auto RandomFileWriteCache::getItem(const char* path) -> WriterPtr {

}

}
