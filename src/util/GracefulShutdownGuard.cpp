
#include <templatious/FullPack.hpp>

#include "GracefulShutdownGuard.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

typedef GracefulShutdownInterface GSI;

std::shared_ptr< GracefulShutdownGuard > GracefulShutdownGuard::makeNew() {
    std::shared_ptr< GracefulShutdownGuard > res(new GracefulShutdownGuard());
    res->_myHandle = res;
    return res;
}

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
    std::vector< StrongMsgPtr > steal(std::move(_vec));


}

void GracefulShutdownGuard::add(const StrongMsgPtr& ptr) {
    auto myselfStrong = _myHandle.lock();
    assert( myselfStrong != nullptr && "HUH?!?" );
    auto msg = SF::vpackPtr<
        GSI::InRegisterItself, StrongMsgPtr
    >(nullptr,myselfStrong);
    ptr->message(msg);
}

void GracefulShutdownGuard::message(const std::shared_ptr< templatious::VirtualPack >& msg) {
    _cache.enqueue(msg);
}

void GracefulShutdownGuard::message(templatious::VirtualPack& msg) {
    assert( false && "Sync messages disabled." );
}

auto GracefulShutdownGuard::genHandler() -> VmfPtr {
    return SF::virtualMatchFunctorPtr(
        SF::virtualMatch< GSI::OutRegisterItself, StrongMsgPtr >(
            [=](GSI::OutRegisterItself,const StrongMsgPtr& msg) {
                SA::add(_vec,msg);
            }
        )
    );
}

}

