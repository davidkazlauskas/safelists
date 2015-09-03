
#include "SafeListDownloader.hpp"

namespace SafeLists {

struct SafeListDownloaderImpl : public Messageable {
    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        // do stuff
    }

    void message(templatious::VirtualPack& msg) override {
        assert( false && "Synchronous messages disabled on SafeListDownloader." );
    }
};

StrongMsgPtr SafeListDownloader::startNew(
        const char* path,
        const StrongMsgPtr& fileWriter,
        const StrongMsgPtr& fileDownloader,
        const WeakMsgPtr& toNotify
)
{

    return nullptr;
}

}

