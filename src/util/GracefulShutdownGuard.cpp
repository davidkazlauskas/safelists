
#include <templatious/FullPack.hpp>

#include "GracefulShutdownGuard.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

typedef GracefulShutdownInterface GSI;

struct GracefulShutdownGuardImpl {
    static void cleanDead(GracefulShutdownGuard& g) {
        bool atleastOne = false;
        TEMPLATIOUS_FOREACH(auto& i,g._vec) {
            auto msg = SF::vpack< GSI::IsDead, bool >(nullptr,true);
            i->message(msg);
            assert( msg.useCount() > 0 && "Message unused..." );
            bool isGone = msg.fGet<1>();
            if (isGone) {
                i = nullptr;
                atleastOne |= true;
            }
        }

        if (atleastOne) {
            SA::clear(
                SF::filter(
                    g._vec,
                    [](const StrongMsgPtr& msg) {
                        return nullptr == msg;
                    }
                )
            );
        }
    }
};

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

    GracefulShutdownGuardImpl::cleanDead(*this);
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

