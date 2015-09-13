#ifndef SAFELISTDOWNLOADER_XE646FNT
#define SAFELISTDOWNLOADER_XE646FNT

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct SafeListDownloader {

    // do we need this?
    //DUMMY_REG(Download,"SLD_Download");

    // messaged out when download
    // is done. Messaged as synchronous message
    // from different thread.
    // Signature:
    // < OutDone >
    DUMMY_REG(OutDone,"SLD_Done");

    // messaged when download is started.
    // Signature:
    // <
    //    OutStarted, int (fileid)
    // >
    DUMMY_REG(OutStarted,"SLD_OutStarted");

    static StrongMsgPtr startNew(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify
    );
};

}

#endif /* end of include guard: SAFELISTDOWNLOADER_XE646FNT */
