
#include "RandomFileWriter.hpp"

namespace SafeLists {

RandomFileWriteHandle::RandomFileWriteHandle(const char* path,int64_t size)
    : _path(path), _size(size)
{

}

}
