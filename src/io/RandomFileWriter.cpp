
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

    // returns if all traversed
    template <class Vec,class Func>
    bool traverseVecStartingAt(Vec& vec,Func&& func,int start,int step = -1) {
        int iterations = SA::size(vec) / std::abs(step);
        int current = start;
        TEMPLATIOUS_REPEAT( iterations ) {
            bool res = func(SA::getByIndex(vec,current),current);
            if (!res) {
                return false;
            }
            current += step;
        }
        return true;
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

std::string RandomFileWriteHandle::getPath() const {
    return _path;
}

struct RandomFileWriteCacheImpl {
    static void advanceCachePoint(RandomFileWriteCache& cache) {
        ++cache._cachePoint;
        if (SA::size(cache._vec) >= cache._cachePoint) {
            cache._cachePoint = 0;
        }
    }
};

RandomFileWriteCache::RandomFileWriteCache(int items)
    : _maxItems(items), _cachePoint(0)
{

}

auto RandomFileWriteCache::getItem(const char* path,int64_t size) -> WriterPtr {
    WriterPtr* out = nullptr;
    int outPos = -1;
    bool res = traverseVecStartingAt(_vec,
        [&](WriterPtr& ptr,int index) {
            if (nullptr != ptr) {
                auto pathNew = ptr->getPath();
                if (pathNew == path) {
                    out = std::addressof(ptr);
                    outPos = index;
                }
            }
            return true;
        },
        _cachePoint
    );

    Helper::advanceCachePoint(*this);

    if (res) {
        auto& ref = SA::getByIndex(_vec,_cachePoint);
        ref = std::make_shared< RandomFileWriteHandle >(path,size);
        return ref;
    }

    if (_cachePoint != outPos) {
        std::swap(_vec[_cachePoint],_vec[outPos]);
    }

    // move item forward to be cache hot
    return *out;
}

bool RandomFileWriteCache::isCached(const char* path) const {
    TEMPLATIOUS_FOREACH(auto& i,_vec) {
        if (i->_path == path) {
            return true;
        }
    }

    return false;
}

}
