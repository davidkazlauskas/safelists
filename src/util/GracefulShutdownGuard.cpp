
#include <templatious/FullPack.hpp>

#include "GracefulShutdownGuard.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

void GracefulShutdownGuard::cleanup() {

}

void GracefulShutdownGuard::waitAll() {

}

void GracefulShutdownGuard::add(const StrongMsgPtr& ptr) {

}

void GracefulShutdownGuard::message(const std::shared_ptr< templatious::VirtualPack >& msg) {

}

void GracefulShutdownGuard::message(templatious::VirtualPack& msg) {

}

auto GracefulShutdownGuard::genHandler() -> VmfPtr {
    typedef GracefulShutdownInterface GSI;
    return SF::virtualMatchFunctorPtr(
        SF::virtualMatch< GSI::OutRegisterItself, StrongMsgPtr >(
            [=](GSI::OutRegisterItself,const StrongMsgPtr& msg) {
            }
        )
    );
}

}

