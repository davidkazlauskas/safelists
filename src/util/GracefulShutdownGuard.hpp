
#ifndef GRACEFULSHUTDOWNGUARD_AAF605J2
#define GRACEFULSHUTDOWNGUARD_AAF605J2

#include <vector>
#include <LuaPlumbing/messageable.hpp>

namespace SafeLists {

    struct GracefulShutdownGuard {

        void cleanup();
        void waitAll();

    private:
        std::vector< StrongMsgPtr > _vec;
    };

}

#endif /* end of include guard: GRACEFULSHUTDOWNGUARD_AAF605J2 */

