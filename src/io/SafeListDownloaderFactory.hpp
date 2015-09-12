#ifndef SAFELISTDOWNLOADERFACTORY_LHKOTFBK
#define SAFELISTDOWNLOADERFACTORY_LHKOTFBK

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct SafeListDownloaderFactory {

    // Create new download, which will notify
    // messeagable object asynchronously.
    // Signature:
    // <
    //   InNewAsync,
    //   std::string (path),
    //   StrongMsgPtr (notify object),
    //   StrongMsgPtr (out session)
    // >
    DUMMY_REG(InNewAsync,"SLDF_InNewAsync");

    // Create safelist session from
    // async sqlite to safelist.
    // Signature:
    // <
    //   CreateSession,
    //   StrongMsgPtr (asyncsqlite to safelist),
    //   std::string (session path)
    // >
    DUMMY_REG(CreateSession,"SLDF_CreateSession");

    static StrongMsgPtr createNew();
};

}

#endif /* end of include guard: SAFELISTDOWNLOADERFACTORY_LHKOTFBK */
