#ifndef LICENSING_X1B9GTMJ
#define LICENSING_X1B9GTMJ

#include <LuaPlumbing/messageable.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct LicenseDaemon {

    // asynchronous check if license is expied.
    // Signature:
    // < IsExpired, bool (out result) >
    DUMMY_REG(IsExpired,"LD_IsExpired");

    // singleton
    static StrongMsgPtr getDaemon();
};

}

#endif /* end of include guard: LICENSING_X1B9GTMJ */
