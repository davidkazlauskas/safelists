
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
    _handler(genHandler()), _stHandler(genStHandler())
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
    GracefulShutdownGuardImpl::cleanDead(*this);

    // this may not be final call,
    // we just take everything we have
    std::vector< StrongMsgPtr > steal(std::move(_vec));

    auto shutdown = SF::vpack< GSI::ShutdownSignal >(nullptr);
    TEMPLATIOUS_FOREACH(auto& i,steal) {
        i->message(shutdown);
    }

    auto wait = SF::vpack< GSI::WaitOut >(nullptr);
    TEMPLATIOUS_FOREACH(auto& i,steal) {
        i->message(wait);
    }
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
    _stHandler->tryMatch(msg);
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

auto GracefulShutdownGuard::genStHandler() -> VmfPtr {
    return SF::virtualMatchFunctorPtr(
        SF::virtualMatch< GSI::AddNew, StrongMsgPtr >(
            [=](GSI::AddNew,const StrongMsgPtr& ptr) {
                this->add(ptr);
            }
        )
    );
}

}

