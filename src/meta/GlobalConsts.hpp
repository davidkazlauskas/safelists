#ifndef GLOBALCONSTS_1I5CHXX9
#define GLOBALCONSTS_1I5CHXX9

#include <LuaPlumbing/messageable.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

    struct GlobalConsts {

        // singleton
        static StrongMsgPtr getConsts();
    };

}

#endif /* end of include guard: GLOBALCONSTS_1I5CHXX9 */
