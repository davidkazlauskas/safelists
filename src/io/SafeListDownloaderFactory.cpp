
#include <templatious/FullPack.hpp>

#include "SafeListDownloaderFactory.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

struct SafeListDownloaderFactoryImpl : public Messageable {

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
    return nullptr;
}

}
