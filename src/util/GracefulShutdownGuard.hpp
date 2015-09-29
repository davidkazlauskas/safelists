
#ifndef GRACEFULSHUTDOWNGUARD_AAF605J2
#define GRACEFULSHUTDOWNGUARD_AAF605J2

#include <vector>
#include <LuaPlumbing/messageable.hpp>

#include "GracefulShutdownInterface.hpp"

namespace SafeLists {

    struct GracefulShutdownGuard {

        void cleanup();
        void waitAll();
        void add(const StrongMsgPtr& ptr);

    private:
        std::vector< StrongMsgPtr > _vec;
        StrongMsgPtr _myHandle;
    };

}

#endif /* end of include guard: GRACEFULSHUTDOWNGUARD_AAF605J2 */

