
#ifndef GRACEFULSHUTDOWNGUARD_AAF605J2
#define GRACEFULSHUTDOWNGUARD_AAF605J2

#include <vector>
#include <LuaPlumbing/messageable.hpp>

#include "GracefulShutdownInterface.hpp"

namespace SafeLists {

    struct GracefulShutdownGuard : public Messageable {

        GracefulShutdownGuard(const GracefulShutdownGuard&) = delete;
        GracefulShutdownGuard(GracefulShutdownGuard&&) = delete;

        void message(const std::shared_ptr< templatious::VirtualPack >& msg) override;
        void message(templatious::VirtualPack& msg) override;

        void processMessages();
        void waitAll();
        void add(const StrongMsgPtr& ptr);

        static std::shared_ptr< GracefulShutdownGuard > makeNew();

    private:
        friend struct GracefulShutdownGuardImpl;

        GracefulShutdownGuard();

        typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;
        VmfPtr genHandler();
        VmfPtr genStHandler();

        std::vector< StrongMsgPtr > _vec;
        VmfPtr _handler;
        VmfPtr _stHandler;
        std::weak_ptr< GracefulShutdownGuard > _myHandle;
        MessageCache _cache;
    };

}

#endif /* end of include guard: GRACEFULSHUTDOWNGUARD_AAF605J2 */

