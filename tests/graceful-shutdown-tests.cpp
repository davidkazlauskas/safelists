
#include <templatious/FullPack.hpp>
#include "catch.hpp"

#include <util/GracefulShutdownGuard.hpp>

TEMPLATIOUS_TRIPLET_STD;

struct Sleep100MsShutdownClass : public Messageable {

    Sleep100MsShutdownClass(const Sleep100MsShutdownClass&) = delete;
    Sleep100MsShutdownClass(Sleep100MsShutdownClass&&) = delete;

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _cache.enqueue(msg);
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "..." );
    }

    std::shared_ptr< Sleep100MsShutdownClass > makeNew() {
        std::shared_ptr< Sleep100MsShutdownClass > res(new Sleep100MsShutdownClass());
        res->_myself = res;
        std::thread(
            [=]() {
                res->mainLoop();
            }
        );
        return res;
    }
private:

    Sleep100MsShutdownClass() :
        _handler(genHandler()), _keepGoing(true) {}

    void mainLoop() {
        for (;;) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );
            if (!_keepGoing) {
                return;
            }
        }
    }

    typedef std::unique_ptr< templatious::VirtualMatchFunctor > VmfPtr;
    typedef SafeLists::GracefulShutdownInterface GSI;

    DUMMY_STRUCT(Shutdown);

    struct MyShutdownGuard : public Messageable {
        MyShutdownGuard(const std::shared_ptr< Sleep100MsShutdownClass >& ref) :
            _master(ref), _handler(genHandler()), _fut(_prom.get_future()) {}

        void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
            assert( false && "..." );
        }

        void message(templatious::VirtualPack& msg) override {
            _handler->tryMatch(msg);
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
                                Shutdown
                            >(nullptr);
                            locked->message(shutdownMsg);
                        }
                    }
                ),
                SF::virtualMatch< GSI::WaitOut >(
                    [=](GSI::WaitOut) {
                        _fut.wait();
                    }
                )
            );
        }

        std::weak_ptr< Sleep100MsShutdownClass > _master;
        VmfPtr _handler;
        std::promise< void > _prom;
        std::future< void > _fut;
    };

    VmfPtr genHandler() {
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                [=](GSI::InRegisterItself,const StrongMsgPtr& ref) {
                    auto myself = _myself.lock();
                    assert( nullptr != myself && "?!?" );
                    std::shared_ptr< MyShutdownGuard > g(new MyShutdownGuard(myself));
                    _guard = g;
                    auto msg = SF::vpackPtr<
                        GSI::OutRegisterItself, StrongMsgPtr >( nullptr, g );
                    ref->message(msg);
                }
            ),
            SF::virtualMatch< Shutdown >(
                [=](Shutdown) {
                    _keepGoing = false;
                }
            )
        );
    }

    MessageCache _cache;
    VmfPtr _handler;
    bool _keepGoing;
    std::weak_ptr< Sleep100MsShutdownClass > _myself;
    std::weak_ptr< MyShutdownGuard > _guard;
};

TEST_CASE("graceful_shutdown_guard_test","[graceful_shutdown]") {
    using namespace SafeLists;

    auto g = GracefulShutdownGuard::makeNew();

}

