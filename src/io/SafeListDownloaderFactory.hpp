#ifndef SAFELISTDOWNLOADERFACTORY_LHKOTFBK
#define SAFELISTDOWNLOADERFACTORY_LHKOTFBK

#include <LuaPlumbing/plumbing.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct SafeListDownloaderFactory {

    static StrongMsgPtr createNew();
};

}

#endif /* end of include guard: SAFELISTDOWNLOADERFACTORY_LHKOTFBK */
