#include "SafeListDownloaderFactory.hpp"

namespace SafeLists {

struct SafeListDownloaderFactoryImpl : public Messageable {

};

StrongMsgPtr SafeListDownloaderFactory::createNew() {
    return nullptr;
}

}
