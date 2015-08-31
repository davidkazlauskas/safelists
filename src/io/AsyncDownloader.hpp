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
        //     Interval, // file interval
        //     std::function< bool(const char*,int64_t,int64_t) >, // byte process function
        //     std::weak_ptr< Messeagable > // handle other misc errors
        // >
        DUMMY_REG(ScheduleDownload,"AD_ScheduleDownload");

        static StrongMsgPtr createNew(const char* type = "imitation");
    };

}

#endif /* end of include guard: ASYNCDOWNLOADER_7J8L0K2A */
