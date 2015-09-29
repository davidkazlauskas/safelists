#ifndef GENERICSHUTDOWNGUARD_OIYM7Y0X
#define GENERICSHUTDOWNGUARD_OIYM7Y0X

#include "GracefulShutdownInterface.hpp"

namespace SafeLists {

typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;
typedef SafeLists::GracefulShutdownInterface GSI;

struct GenericShutdownGuard : public Messageable {

    DUMMY_STRUCT(SetFuture);
    DUMMY_STRUCT(ShutdownTarget);

    GenericShutdownGuard() :
        _handler(genHandler()),
        _fut(_prom.get_future()) {}

    void message(templatious::VirtualPack& pack) override {
        _handler->tryMatch(pack);
    }

    void message(const StrongPackPtr& pack) override {
        assert( false && "Async message disabled." );
    }

    void setPromise() {
        _prom.set_value();
    }

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< GSI::IsDead, bool >(
                [=](GSI::IsDead,bool& res) {
                    auto locked = _master.lock();
                    res = nullptr == locked;
                }
            ),
            SF::virtualMatch< GSI::ShutdownSignal >(
                [=](GSI::ShutdownSignal) {
                    auto locked = _master.lock();
                    if (locked == nullptr) {
                        _prom.set_value();
                    } else {
                        auto shutdownMsg = SF::vpackPtr<
                            ShutdownTarget
                        >(nullptr);
                        locked->message(shutdownMsg);
                    }
                }
            ),
            SF::virtualMatch< GSI::WaitOut >(
                [=](GSI::WaitOut) {
                    _fut.wait();
                }
            ),
            SF::virtualMatch< SetFuture >(
                [=](SetFuture) {
                    _prom.set_value();
                }
            )
        );
    }

    void setMaster(const WeakMsgPtr& ptr) {
        _master = ptr;
    }

private:
    VmfPtr _handler;
    std::promise< void > _prom;
    std::future< void > _fut;
    WeakMsgPtr _master;
};

}

#endif /* end of include guard: GENERICSHUTDOWNGUARD_OIYM7Y0X */

