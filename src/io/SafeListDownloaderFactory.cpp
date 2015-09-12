
#include <sqlite3.h>

#include <templatious/FullPack.hpp>
#include <io/SafeListDownloader.hpp>

#include "SafeListDownloaderFactory.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    // returns in memory database
    // which needs to be saved to file.
    sqlite3* createDownloadSession(sqlite3* connection) {
        sqlite3* result = nullptr;
        sqlite3_open(":memory:",&result);
        return result;
    }
}

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
                [](SLDF::InNewAsync,
                    const std::string& path,
                    const StrongMsgPtr& toNotify,
                    StrongMsgPtr& output)
                {
                }
            ),
            SF::virtualMatch<
                SLDF::CreateSession,
                StrongMsgPtr,
                StrongMsgPtr,
                std::string
            >(
                [](SLDF::CreateSession,
                   StrongMsgPtr& asyncSqlite,
                   StrongMsgPtr& notifyWhenDone,
                   const std::string& path)
                {
                    WeakMsgPtr notify = notifyWhenDone;

                    typedef std::function<void(sqlite3*)> Sig;

                    auto asyncMessage = SF::vpackPtrWCallback<
                        Sig
                    >(
                        [=](const TEMPLATIOUS_VPCORE<Sig>& pack) {
                            auto locked = notify.lock();
                            if (nullptr == locked) {
                                return;
                            }

                            auto notifyMsg = SF::vpackPtr<
                                SLDF::OutCreateSessionDone
                            >(nullptr);
                            locked->message(notifyMsg);
                        },
                        [=](sqlite3* connection) {
                            sqlite3* memSession = createDownloadSession(connection);
                        }
                    );
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
