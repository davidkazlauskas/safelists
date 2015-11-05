
#include <cassert>
#include <mutex>

#include "ProgramArgs.hpp"

namespace {

    typedef std::lock_guard< std::mutex > LGuard;

    static std::vector< std::string > theArgArray;
    static std::mutex theArgMutex;

}

namespace SafeLists {

void setGlobalProgramArgs(char** argv,int argc) {
    static bool isSet = false;

    LGuard g(theArgMutex);

    assert( isSet && "Arguments should not be set yet." );
    for (int i = 0; i < argc; ++i) {
        theArgArray.push_back(argv[i]);
    }
    isSet = true;
}

std::vector< std::string > getGlobalProgramArgs() {
    LGuard g(theArgMutex);
    std::vector< std::string > res = theArgArray;
    return res;
}

}

