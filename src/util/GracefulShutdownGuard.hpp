
#ifndef GRACEFULSHUTDOWNGUARD_AAF605J2
#define GRACEFULSHUTDOWNGUARD_AAF605J2

#include <vector>
#include <LuaPlumbing/messageable.hpp>

#include "GracefulShutdownInterface.hpp"

namespace SafeLists {

    struct GracefulShutdownGuard : public Messageable {

        void message(const std::shared_ptr< templatious::VirtualPack >& msg) override;
        void message(templatious::VirtualPack& msg) override;

        void cleanup();
        void waitAll();
        void add(const StrongMsgPtr& ptr);

    private:
        std::vector< StrongMsgPtr > _vec;
        std::weak_ptr< GracefulShutdownGuard > _myHandle;
    };

}

#endif /* end of include guard: GRACEFULSHUTDOWNGUARD_AAF605J2 */

