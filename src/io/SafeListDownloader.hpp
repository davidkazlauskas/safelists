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
    // < OutProgressUpdate, int (id), double (bytes done), double (bytes total) >
    DUMMY_REG(OutProgressUpdate,"SLD_OutProgressUpdate");

    // messaged when hash is updated
    // Signature:
    // < OutHashUpdate, int (id), std::string (hash) >
    DUMMY_REG(OutHashUpdate,"SLD_OutHashUpdate");

    // messaged when file size is updated
    // Signature:
    // < OutSizeUpdate, int (id), double (size) >
    DUMMY_REG(OutSizeUpdate,"SLD_OutSizeUpdate");

    // messaged when total downloads of session are known
    // Signature:
    // < OutTotalDownloads, int (count) >
    DUMMY_REG(OutTotalDownloads,"SLDF_OutTotalDownloads");

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
