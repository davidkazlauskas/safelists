
#include <cassert>
#include "Licensing.hpp"

namespace SafeLists {

struct LicenseDaemonDummyImpl : public Messageable {

    // this is for sending message across threads
    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _cache.enqueue(msg);
    }

    // this is for sending stack allocated (faster)
    // if we know we're on the same thread as GUI
    void message(templatious::VirtualPack& msg) override {
        assert( false && "Only async messages in license daemon." );
    }

private:
    MessageCache _cache;
};

StrongMsgPtr LicenseDaemon::getDaemon() {
    static StrongMsgPtr theObject = nullptr;
    return nullptr;
}

}

