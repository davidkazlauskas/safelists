#ifndef SAFELISTDOWNLOADER_XE646FNT
#define SAFELISTDOWNLOADER_XE646FNT

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct SafeListDownloader {


    static StrongMsgPtr startNew(
            const char* path,
            const StrongMsgPtr& fileWriter,
            const StrongMsgPtr& fileDownloader,
            const WeakMsgPtr& toNotify
    );
};

}

#endif /* end of include guard: SAFELISTDOWNLOADER_XE646FNT */
