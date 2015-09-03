#ifndef RANDOMFILEWRITER_MCTC8IU8
#define RANDOMFILEWRITER_MCTC8IU8

#include <cstdio>
#include <string>
#include <vector>
#include <memory>

#include <templatious/util/Exceptions.hpp>

namespace SafeLists {

TEMPLATIOUS_BOILERPLATE_EXCEPTION(
    RandomFileWriterDifferentFileSizeException,
    "File opened does not correspond in size with the size expected."
);

TEMPLATIOUS_BOILERPLATE_EXCEPTION(
    RandomFileWriterOutOfBoundsWriteException,
    "File position is out of bounds."
);

struct RandomFileWriteHandle {

    RandomFileWriteHandle(const char* path,int64_t size);
    ~RandomFileWriteHandle();

    void write(const char* buffer,int64_t start,int64_t size);
    void read(char* buffer,int64_t start,int64_t size);
    std::string getPath() const;

    friend struct RandomFileWriteCache;
private:
    std::string _path;
    int64_t _size;
    int64_t _writeSize;
    FILE* _handle;
};

TEMPLATIOUS_BOILERPLATE_EXCEPTION(
    RandomFileWriteCacheDifferentSize,
    "File from cache differs in size with requested size."
);

// WARNING:
// handles returned by this class should not travel
// across universe because otherwise duplicate newly
// created handles will be returned by this class
struct RandomFileWriteCache {
    typedef std::shared_ptr< RandomFileWriteHandle > WriterPtr;

    RandomFileWriteCache(int items = 16);
    WriterPtr getItem(const char* path,int64_t size);
    bool isCached(const char* path) const;
    void clear();

    friend struct RandomFileWriteCacheImpl;
    typedef struct RandomFileWriteCacheImpl Helper;
private:
    int _maxItems;
    int _cachePoint;
    std::vector< WriterPtr > _vec;
};

}

#endif /* end of include guard: RANDOMFILEWRITER_MCTC8IU8 */
