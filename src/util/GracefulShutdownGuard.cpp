
#include <templatious/FullPack.hpp>

#include "GracefulShutdownGuard.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

GracefulShutdownGuard::GracefulShutdownGuard() :
    _handler(genHandler())
{
}

void GracefulShutdownGuard::processMessages() {
    _cache.process(
        [&](templatious::VirtualPack& pack) {
            _handler->tryMatch(pack);
        }
    );
}

void GracefulShutdownGuard::waitAll() {

}

void GracefulShutdownGuard::add(const StrongMsgPtr& ptr) {

}

void GracefulShutdownGuard::message(const std::shared_ptr< templatious::VirtualPack >& msg) {
    _cache.enqueue(msg);
}

void GracefulShutdownGuard::message(templatious::VirtualPack& msg) {
    assert( false && "Sync messages disabled." );
}

auto GracefulShutdownGuard::genHandler() -> VmfPtr {
    typedef GracefulShutdownInterface GSI;
    return SF::virtualMatchFunctorPtr(
        SF::virtualMatch< GSI::OutRegisterItself, StrongMsgPtr >(
            [=](GSI::OutRegisterItself,const StrongMsgPtr& msg) {
                SA::add(_vec,msg);
            }
        )
    );
}

}

