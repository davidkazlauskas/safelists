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
    DUMMY_REG(OutDone,"SLD_OutDone");

    // messaged when download is started.
    // Signature:
    // <
    //    OutStarted, int (fileid), std::string (path)
    // >
    DUMMY_REG(OutStarted,"SLD_OutStarted");

    // messaged when download is started.
    // Signature:
    // <
    //    OutSingleDone, int (fileid)
    // >
    DUMMY_REG(OutSingleDone,"SLD_OutSingleDone");

    // messaged when progress moved
    // Signature:
    // < OutProgressUpdate, double (bytes done), double (bytes total) >
    DUMMY_REG(OutProgressUpdate,"SLD_OutProgressUpdate");

    static StrongMsgPtr startNew(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const StrongMsgPtr& toNotify,
        bool notifyAsAsync = false
    );
};

}

#endif /* end of include guard: SAFELISTDOWNLOADER_XE646FNT */
