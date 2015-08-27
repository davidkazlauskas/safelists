#ifndef ASYNCDOWNLOADER_7J8L0K2A
#define ASYNCDOWNLOADER_7J8L0K2A

#include <LuaPlumbing/plumbing.hpp>
#include <util/DummyStruct.hpp>

namespace SafeLists {

    struct AsyncDownloader {

        static StrongMsgPtr createNew(const char* type = "imitation");
    };

}

#endif /* end of include guard: ASYNCDOWNLOADER_7J8L0K2A */
