#ifndef LICENSING_X1B9GTMJ
#define LICENSING_X1B9GTMJ

#include <LuaPlumbing/messageable.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct LicenseDaemon {

    // singleton
    static StrongMsgPtr getDaemon();
};

}

#endif /* end of include guard: LICENSING_X1B9GTMJ */
