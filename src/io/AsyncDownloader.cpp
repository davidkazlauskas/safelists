
#include <cstring>

#include "AsyncDownloader.hpp"

namespace SafeLists {

    struct AsyncDownloaderImitationImpl : public Messageable {
        // this is for sending message across threads
        void message(const std::shared_ptr< templatious::VirtualPack >& msg) {

        }

        // this is for sending stack allocated (faster)
        // if we know we're on the same thread as GUI
        void message(templatious::VirtualPack& msg) {

        }
    };

    StrongMsgPtr AsyncDownloader::createNew(const char* type) {
        if (0 == strcmp(type,"imitation")) {
            return std::make_shared< AsyncDownloaderImitationImpl >();
        }
        return nullptr;
    }

}
