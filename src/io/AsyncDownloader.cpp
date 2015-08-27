
#include <cstring>
#include <future>

#include <templatious/FullPack.hpp>

#include "AsyncDownloader.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

    struct AsyncDownloaderImitationImpl : public Messageable {
        AsyncDownloaderImitationImpl() : _shutdown(false) {}

        // this is for sending message across threads
        void message(const std::shared_ptr< templatious::VirtualPack >& msg) {
            _cache.enqueue(msg);
        }

        // this is for sending stack allocated (faster)
        // if we know we're on the same thread as GUI
        void message(templatious::VirtualPack& msg) {
            assert( false && "Synchronous messages disabled for this object." );
        }

        static StrongMsgPtr spinupNew() {
            auto result = std::make_shared< AsyncDownloaderImitationImpl >();

            std::thread(
                [=]() {
                    result->messageLoop();
                }
            ).detach();

            return result;
        }

    private:
        typedef std::unique_ptr< templatious::VirtualMatchFunctor > Handler;

        void messageLoop() {
            while (!_shutdown) {
                _cache.process(
                    [=](templatious::VirtualPack& p) {
                        this->_handler->tryMatch(p);
                    }
                );
            }
        }

        void shutdown() {
            _shutdown = true;
        }

        Handler genHandler() {
            typedef AsyncDownloader AD;
            return SF::virtualMatchFunctorPtr(
                SF::virtualMatch< AD::Shutdown >(
                    [=](AD::Shutdown) {
                        this->shutdown();
                    }
                )
            );
        }

        MessageCache _cache;
        bool _shutdown;
        Handler _handler;
    };

    StrongMsgPtr AsyncDownloader::createNew(const char* type) {
        if (0 == strcmp(type,"imitation")) {
            return AsyncDownloaderImitationImpl::spinupNew();
        }
        return nullptr;
    }

}
