
#include <templatious/FullPack.hpp>
#include <io/SafeListDownloader.hpp>

#include "SafeListDownloaderFactory.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

struct SafeListDownloaderFactoryImpl : public Messageable {

    SafeListDownloaderFactoryImpl() : _handler(genHandler())
    {}

    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        assert( false &&
            "Asynchronous messages disabled for SafeListDownloaderFactoryImpl." );
    }

    void message(templatious::VirtualPack& msg) override {
        _handler->tryMatch(msg);
    }

private:
    VmfPtr genHandler() {
        typedef SafeListDownloaderFactory SLDF;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch<
                SLDF::InNewAsync,
                std::string, // path
                StrongMsgPtr, // to notify
                StrongMsgPtr // out object
            >(
                [=](SLDF::InNewAsync,
                    const std::string& path,
                    const StrongMsgPtr& toNotify,
                    StrongMsgPtr& output)
                {

                }
            )
        );
    }

    VmfPtr _handler;
};

StrongMsgPtr SafeListDownloaderFactory::createNew() {
    return std::make_shared< SafeListDownloaderFactoryImpl >();
}

}
