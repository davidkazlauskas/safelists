#ifndef ASYNCDOWNLOADER_7J8L0K2A
#define ASYNCDOWNLOADER_7J8L0K2A

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

    struct AsyncDownloader {

        DUMMY_REG(Shutdown,"AD_Shutdown");

        static StrongMsgPtr createNew(const char* type = "imitation");
    };

}

#endif /* end of include guard: ASYNCDOWNLOADER_7J8L0K2A */
