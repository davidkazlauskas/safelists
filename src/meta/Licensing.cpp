
#include <cassert>
#include <sodium.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include <util/Semaphore.hpp>
#include <util/GenericShutdownGuard.hpp>
#include <util/ScopeGuard.hpp>

#include "Licensing.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef SafeLists::GracefulShutdownInterface GSI;

namespace SafeLists {

std::string getCurrentUserIdBase64() {
    // dummy implementation, matches play backend test
    return "4j5iMF/54gJW/c4bPAdN28+4YcIeSQj3QGvOv2xIZjU=";
}

int querySignature(const std::string& user,char* out,int buflen) {
    CURL* handle = ::curl_easy_init();
    auto clean = SCOPE_GUARD_LC(
        ::curl_easy_cleanup(handle);
    );

    return 0;
}

struct LicenseDaemonDummyImpl : public Messageable {
    LicenseDaemonDummyImpl(const LicenseDaemonDummyImpl&) = delete;
    LicenseDaemonDummyImpl(LicenseDaemonDummyImpl&&) = delete;

    // this is for sending message across threads
    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _cache.enqueue(msg);
        _sem.notify();
    }

    // this is for sending stack allocated (faster)
    // if we know we're on the same thread as GUI
    void message(templatious::VirtualPack& msg) override {
        assert( false && "Only async messages in license daemon." );
    }

    static std::shared_ptr< LicenseDaemonDummyImpl > makeNew() {
        std::shared_ptr< LicenseDaemonDummyImpl > result(
            new LicenseDaemonDummyImpl()
        );
        result->_myself = result;
        std::thread(
            [=]() {
                result->mainLoop();
            }
        ).detach();
        return result;
    }
private:
    void mainLoop() {
        auto sendMsg = SCOPE_GUARD_LC(
            auto locked = _notifyExit.lock();
            if (nullptr != locked) {
                auto msg = SF::vpack<
                    GenericShutdownGuard::SetFuture >(nullptr);
                locked->message(msg);
            }
        );

        while (_keepGoing) {
            _sem.wait();
            _cache.process(
                [=](templatious::VirtualPack& pack) {
                    _handler->tryMatch(pack);
                }
            );
        }
    }

    LicenseDaemonDummyImpl() :
        _handler(genHandler()), _keepGoing(true) {}

    VmfPtr genHandler() {
        typedef LicenseDaemon LD;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< LD::IsExpired, bool >(
                [=](ANY_CONV,bool& res) {
                    // do something, read files or whatever
                    return res = false;
                }
            ),
            SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                [=](ANY_CONV,StrongMsgPtr& ptr) {
                    auto handler = std::make_shared< GenericShutdownGuard >();
                    handler->setMaster(_myself);
                    auto msg = SF::vpackPtr< GSI::OutRegisterItself, StrongMsgPtr >(
                        nullptr, handler
                    );
                    assert( nullptr == _notifyExit.lock() );
                    _notifyExit = handler;
                    ptr->message(msg);
                }
            ),
            SF::virtualMatch< GenericShutdownGuard::ShutdownTarget >(
                [=](GenericShutdownGuard::ShutdownTarget) {
                    this->_keepGoing = false;
                }
            )
        );
    }

    MessageCache _cache;
    StackOverflow::Semaphore _sem;
    VmfPtr _handler;
    bool _keepGoing;
    WeakMsgPtr _myself;
    WeakMsgPtr _notifyExit;
};

StrongMsgPtr LicenseDaemon::getDaemon() {
    static StrongMsgPtr theObject =
        LicenseDaemonDummyImpl::makeNew();
    return theObject;
}

}

