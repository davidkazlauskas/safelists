#ifndef RANDOMFILEWRITER_MCTC8IU8
#define RANDOMFILEWRITER_MCTC8IU8

#include <string>
#include <cstdio>

namespace SafeLists {

struct RandomFileWriteHandle {

    RandomFileWriteHandle(const char* path,int64_t size);
    ~RandomFileWriteHandle();

private:
    std::string _path;
    int64_t _size;
    FILE* _handle;
};

}

#endif /* end of include guard: RANDOMFILEWRITER_MCTC8IU8 */
