
#include <fstream>
#include <templatious/FullPack.hpp>

#include "RandomFileWriter.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {

    bool fileRangeCheck(int64_t start,int64_t size,int64_t max) {
        if (start < 0) {
            return false;
        }

        if (max != -1 && start + size > max) {
            return false;
        }

        return true;
    }

    // returns if all traversed
    template <class Vec,class Func>
    bool traverseVecStartingAt(Vec& vec,Func&& func,int start,int step = -1) {
        int vecSize = SA::size(vec);
        int iterations =  vecSize / std::abs(step);
        int current = start;
        TEMPLATIOUS_REPEAT( iterations ) {
            bool res = func(SA::getByIndex(vec,current),current);
            if (!res) {
                return false;
            }
            current += step;
            if (current < 0) {
                current = vecSize - 1;
            } else if (current >= vecSize) {
                current = 0;
            }
        }
        return true;
    }
}

namespace SafeLists {


RandomFileWriteHandle::RandomFileWriteHandle(const char* path,int64_t size)
    : _path(path), _size(size), _writeSize(0)
{
    FILE* handle = fopen(_path.c_str(),"r+");
    if (nullptr != handle) {
        fseek(handle,0,SEEK_END);
        int64_t realSize = ftell(handle);
        if (_size > 0) {
            if (realSize != _size) {
                throw RandomFileWriterDifferentFileSizeException();
            }
        }
        _writeSize = realSize;
    } else {
        handle = fopen(_path.c_str(),"w+");
        if (_size > 0) {
            fseek(handle,_size - 1,SEEK_SET);
            fputc('7',handle);
        }
    }

    _handle = handle;
}

RandomFileWriteHandle::~RandomFileWriteHandle() {
    fclose(_handle);
}

void RandomFileWriteHandle::write(const char* buffer,int64_t start,int64_t size) {
    if (!fileRangeCheck(start,size,_size)) {
        throw RandomFileWriterOutOfBoundsWriteException();
    }

    auto outS = fseek(_handle,start,SEEK_SET);
    auto outW = fwrite(buffer,size,1,_handle);
    assert( outS == 0 && outW > 0 );

    int64_t farthestOffset = start + size;
    if (farthestOffset > _writeSize) {
        _writeSize = farthestOffset;
    }
}

void RandomFileWriteHandle::read(char* buffer,int64_t start,int64_t size) {
    if (!fileRangeCheck(start,size,_size)) {
        throw RandomFileWriterOutOfBoundsWriteException();
    }

    auto outS = fseek(_handle,start,SEEK_SET);
    auto outR = fread(buffer,size,1,_handle);
    assert( outS == 0 && outR > 0 );
}

std::string RandomFileWriteHandle::getPath() const {
    return _path;
}

struct RandomFileWriteCacheImpl {
    static void advanceCachePoint(RandomFileWriteCache& cache) {
        ++cache._cachePoint;
        if (SA::size(cache._vec) <= cache._cachePoint) {
            cache._cachePoint = 0;
        }
    }
};

RandomFileWriteCache::RandomFileWriteCache(int items)
    : _maxItems(items), _cachePoint(0), _vec(_maxItems)
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
                    if (size != ptr->_size) {
                        throw RandomFileWriteCacheDifferentSize();
                    }
                    out = std::addressof(ptr);
                    outPos = index;
                    return false;
                }
            }
            return true;
        },
        _cachePoint
    );

    if (_cachePoint == outPos) {
        // head of the cache
        return _vec[_cachePoint];
    }

    Helper::advanceCachePoint(*this);
    int currCachePoint = _cachePoint;

    if (res) {
        auto& ref = SA::getByIndex(_vec,currCachePoint);
        ref = std::make_shared< RandomFileWriteHandle >(path,size);
        return ref;
    }

    std::swap(_vec[currCachePoint],_vec[outPos]);

    // move item forward to be cache hot
    return *out;
}

bool RandomFileWriteCache::isCached(const char* path) const {
    TEMPLATIOUS_FOREACH(auto& i,_vec) {
        if (nullptr != i && i->_path == path) {
            return true;
        }
    }

    return false;
}

void RandomFileWriteCache::clear() {
    SM::set(nullptr,_vec);
}

}
