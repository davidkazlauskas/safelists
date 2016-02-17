#ifndef ASYNCDOWNLOADER_7J8L0K2A
#define ASYNCDOWNLOADER_7J8L0K2A

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

    struct AsyncDownloader {

        // Shutdown
        DUMMY_REG(Shutdown,"AD_Shutdown");

        // Schedule.
        // Signature: <
        //     ScheduleDownload,
        //     const std::string, // path
        //     Interval, // file interval
        //     std::function< bool(const char*,int64_t,int64_t) >, // byte process function
        //     std::weak_ptr< Messageable > // handle other misc errors
        // >
        //
        // Signature: <
        //     ScheduleDownload,
        //     const std::string, // path
        //     IntervalList, // file interval, for unfinished, partially downloaded files
        //     std::function< bool(const char*,int64_t,int64_t) >, // byte process function
        //     std::weak_ptr< Messageable > // handle other misc errors
        // >
        DUMMY_REG(ScheduleDownload,"AD_ScheduleDownload");

        // Signalled when download size is known.
        // Signature: < OutSizeKnown, int64_t (file size) >
        DUMMY_REG(OutSizeKnown,"AD_OutSizeKnown");

        // Signalled when download is finished.
        // Signature: < OutDownloadFinished >
        DUMMY_REG(OutDownloadFinished,"AD_OutDownloadFinished");

        // Signalled when file is not found.
        // Signature: < OutDownloadFinished >
        DUMMY_REG(OutFileNotFound,"AD_OutFileNotFound");

        static StrongMsgPtr createNew(const char* type = "imitation");
    };

}

#endif /* end of include guard: ASYNCDOWNLOADER_7J8L0K2A */
