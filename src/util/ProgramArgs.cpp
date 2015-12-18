
#include <cassert>
#include <mutex>
#include <boost/filesystem.hpp>

#include "ProgramArgs.hpp"

namespace {

    namespace fs = boost::filesystem;

    typedef std::lock_guard< std::mutex > LGuard;

    static std::vector< std::string > theArgArray;
    static std::mutex theArgMutex;

}

namespace SafeLists {

void setGlobalProgramArgs(int argc,char** argv) {
    static bool isSet = false;

    LGuard g(theArgMutex);

    assert( !isSet && "Arguments should not be set yet." );
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

std::string executablePath() {
    auto programArgs = SafeLists::getGlobalProgramArgs();
    fs::path selfpath = programArgs[0];
    selfpath = fs::absolute(selfpath.remove_filename());
    return selfpath.string();
}

}

