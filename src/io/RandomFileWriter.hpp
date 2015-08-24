#ifndef RANDOMFILEWRITER_MCTC8IU8
#define RANDOMFILEWRITER_MCTC8IU8

#include <string>
#include <cstdio>

#include <templatious/util/Exceptions.hpp>

namespace SafeLists {

TEMPLATIOUS_BOILERPLATE_EXCEPTION(
    RandomFileWriterDifferentFileSizeException,
    "File opened does not correspond in size with the size expected."
);

struct RandomFileWriteHandle {

    RandomFileWriteHandle(const char* path,int64_t size);
    ~RandomFileWriteHandle();

    void write(const char* buffer,int64_t start,int64_t size);
    void read(char* buffer,int64_t start,int64_t size);

private:
    std::string _path;
    int64_t _size;
    FILE* _handle;
};

}

#endif /* end of include guard: RANDOMFILEWRITER_MCTC8IU8 */
