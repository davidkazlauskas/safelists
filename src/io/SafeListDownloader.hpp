#ifndef SAFELISTDOWNLOADER_XE646FNT
#define SAFELISTDOWNLOADER_XE646FNT

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct SafeListDownloader {

    // do we need this?
    //DUMMY_REG(Download,"SLD_Download");

    DUMMY_REG(OutDone,"SLD_Done");

    static StrongMsgPtr startNew(
            const char* path,
            const StrongMsgPtr& fileWriter,
            const StrongMsgPtr& fileDownloader,
            const StrongMsgPtr& toNotify
    );
};

}

#endif /* end of include guard: SAFELISTDOWNLOADER_XE646FNT */
